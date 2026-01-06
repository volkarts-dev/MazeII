// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "World.hpp"

#include "Application.hpp"
#include "CommonComponents.hpp"
#include "DynamicTree.hpp"
#include "Functions.hpp"
#include "Math.hpp"
#include "PhysComponents.hpp"
#include "SATDetector.hpp"
#include "Solver.hpp"
#include <glm/gtx/norm.hpp>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

namespace ngn {

namespace {

class NodeInfo
{
public:
    Shape origShape;
    uint32_t nodeId;
};

} // namespace

World::World(Application* app) :
    app_{app},
    registry_{app->registry()},
    dynamicTree_{new DynamicTree{registry_}},
    config_{}
{
}

World::~World()
{
    delete dynamicTree_;
}

void World::setConfig(WorldConfig config)
{
    config_ = std::move(config);
}

void World::createBody(entt::entity entity, const BodyCreateInfo& createInfo, Shape shape)
{
    if (createInfo.dynamic)
    {
        registry_->emplace<LinearVelocity>(entity);
        registry_->emplace<LinearForce>(entity);
        registry_->emplace<AngularVelocity>(entity);
        registry_->emplace<AngularForce>(entity);
    }

    if (const auto* pos = registry_->try_get<Position>(entity); !pos)
        registry_->emplace<Position>(entity, glm::vec2{});

    registry_->emplace<TransformChanged>(entity, true);

    registry_->emplace<Body>(entity, Body{
                                 .invMass = createInfo.invMass,
                                 .friction = createInfo.friction,
                                 .restitution = createInfo.restitution,
                             });

    registry_->emplace<Shape>(entity, shape);

    const auto nodeId = dynamicTree_->addObject(calculateAABB(shape), entity);
    registry_->emplace<NodeInfo>(entity, shape, nodeId);
}

void World::update(float deltaTime)
{
    integrate(deltaTime);
    const auto moved = updateTree();
    const auto possibleCollisions = findPossibleCollisions(moved);
    const auto collisions = findActualCollsions(possibleCollisions);
    solveCollisions(collisions);
}

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)

void World::debugDrawState(DebugRenderer* debugRenderer, bool shapes, bool boundingBoxes, bool tree, bool collisions)
{
    auto drawShape = [](DebugRenderer* renderer, const Shape& shape, const glm::vec4& color)
    {
        switch (shape.type)
        {
            using enum Shape::Type;

            case Circle:
                renderer->drawCircle(shape.circle.center, shape.circle.radius, color);
                break;

            case Line:
                renderer->drawLine(shape.line.start, shape.line.end, color);
                break;

            case Capsule:
                renderer->drawCapsule(shape.capsule.start, shape.capsule.end, shape.capsule.radius, color);
                break;
        }
    };

    auto fillShape = [](DebugRenderer* renderer, const Shape& shape, const glm::vec4& color)
    {
        switch (shape.type)
        {
            using enum Shape::Type;

            case Circle:
                renderer->fillCircle(shape.circle.center, shape.circle.radius, color);
                break;

            case Line:
                renderer->drawLine(shape.line.start, shape.line.end, color);
                break;

            case Capsule:
                renderer->fillCapsule(shape.capsule.start, shape.capsule.end, shape.capsule.radius, color);
                break;
        }
    };

    if (shapes)
    {
        for (auto [e, shape]: registry_->view<Shape>().each())
        {
            fillShape(debugRenderer, shape, {0, 1, 0, 0.1});
        }
    }

    if (boundingBoxes)
    {
        dynamicTree_->walkTree([debugRenderer, tree](const TreeNode& node)
        {
            if (node.isLeaf() || tree)
                debugRenderer->drawAABB(node.aabb.topLeft, node.aabb.bottomRight, {1, 0, 1, 0.3});
        });
    }

    if (collisions)
    {
        if (boundingBoxes)
        {
            for (const auto& col : debugPossibleCollisions_)
            {
                debugRenderer->drawAABB(col.second.aabb1.topLeft, col.second.aabb1.bottomRight, {1, 1, 0, 0.6});
                debugRenderer->drawAABB(col.second.aabb2.topLeft, col.second.aabb2.bottomRight, {1, 1, 0, 0.6});
            }
        }

        for (const auto& col : debugCollisions_)
        {
            drawShape(debugRenderer, registry_->get<Shape>(col.first.bodyA), {1, 0, 0, 0.9});
            drawShape(debugRenderer, registry_->get<Shape>(col.first.bodyB), {1, 0, 0, 0.9});
            const auto penVec = col.second.direction * col.second.penetration;
            const auto start = col.second.point - penVec / 2.f;
            const auto end = start + penVec;
            debugRenderer->drawCircle(col.second.point, 2.f, {1, 0, 0, 0.9});
            debugRenderer->drawCircle(start, 1.f, {1, 0, 0, 0.9});
            debugRenderer->drawLine(start, end, {1, 0, 0, 0.9});
        }
    }
}

void World::integrate(float deltaTime)
{
    auto linForces = registry_->view<LinearForce, LinearVelocity>();
    for (auto [e, force, velocity] : linForces.each())
    {
        force.value += config_.gravity;

        const auto veloLen2 = glm::length2(velocity.value);
        if (veloLen2 > 100.f)
        {
            const auto resistance = -(velocity.value / glm::sqrt(veloLen2)) * veloLen2;
            force.value += resistance * config_.linearDamping;
        }
        else if (nearZero(force.value))
        {
            velocity.value = {};
        }

        velocity.value += force.value * deltaTime;

        force.value = {};
    }

    auto angForces = registry_->view<AngularForce, AngularVelocity>();
    for (auto [e, force, velocity] : angForces.each())
    {
        const auto veloLen2 = velocity.value * velocity.value;
        if (veloLen2 > 2.f)
        {
            const auto resistance = -glm::sign(velocity.value) * veloLen2;
            force.value += resistance * config_.angularDamping;
        }
        else if (nearZero(force.value))
        {
            velocity.value = {};
        }

        velocity.value += force.value * deltaTime;
        force.value = 0.f;
    }

    auto linVelocities = registry_->view<LinearVelocity, Position, TransformChanged>();
    for (auto [e, velocity, position, tc] : linVelocities.each())
    {
        const auto newPos = position.value + velocity.value * deltaTime;
        if (newPos != position.value)
        {
            tc.value = true;
            position.value = newPos;
        }
    }

    auto angVelocities = app_->registry()->view<AngularVelocity, Rotation, TransformChanged>();
    for (auto [e, velocity, rotation, tc] : angVelocities.each())
    {
        const auto newRot = rotation.angle + velocity.value * deltaTime;
        if (newRot != rotation.angle)
        {
            tc.value = true;
            rotation.angle = newRot;
        }
        if (tc.value)
            rotation.update();
    }
}

MovedList World::updateTree()
{
    MovedList moved{app_->createFrameAllocator<uint32_t>()};

    auto view = registry_->view<Position, Rotation, Scale, TransformChanged, Body, Shape, NodeInfo>().each();
    for (auto [e, pos, rot, sca, tc, body, shape, nodeInfo] : view)
    {
        if (tc.value)
        {
            shape = transform(nodeInfo.origShape, pos, rot, sca);
            auto aabb = calculateAABB(shape);

            dynamicTree_->updateObject(nodeInfo.nodeId, aabb);

            // only dynamic bodies can move
            if (registry_->all_of<LinearVelocity>(e))
                moved.push_back(nodeInfo.nodeId);
        }

        tc.value = false;
    }

    return moved;
}

CollisionPairSet World::findPossibleCollisions(const MovedList& moved)
{
    CollisionPairSet collisionPairs{app_->createFrameAllocator<CollisionPair>()};
    collisionPairs.reserve(moved.size());

    for (const auto index : moved)
    {
        const auto& node = dynamicTree_->node(index);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        // NOTE This needs stable iterators
        for (auto it = debugPossibleCollisions_.begin(); it != debugPossibleCollisions_.end(); )
        {
            if (it->first.contains(node.entity))
            {
                debugCollisions_.erase(it->first);
                it = debugPossibleCollisions_.erase(it);
            }
            else
            {
                it++;
            }
        }
#endif

        auto callback = [&collisionPairs, &node
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
                , this
#endif
                ](entt::entity entity, const AABB& aabb)
        {
            if (entity != node.entity)
            {
                CollisionPair pair = {
                    .bodyA = node.entity,
                    .bodyB = entity,
                };

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
                debugPossibleCollisions_.insert(std::make_pair(pair, AABBPair{node.aabb, aabb}));
#endif

                collisionPairs.insert(std::move(pair));
            }
            return true;
        };

        dynamicTree_->query(node.aabb, callback);
    }

    return collisionPairs;
}

CollisionList World::findActualCollsions(const CollisionPairSet& collisionPairs)
{
    CollisionList collisions{app_->createFrameAllocator<Collision>()};
    collisions.reserve(collisionPairs.size());

    for (const auto& col : collisionPairs)
    {
        const auto& shapeA = registry_->get<Shape>(col.bodyA);
        const auto& shapeB = registry_->get<Shape>(col.bodyB);

        Collision collision{.pair = col};
        SATDetector::testCollision(collision, shapeA, shapeB);
        if (collision.colliding)
        {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
            debugCollisions_.insert_or_assign(col, collision);
#endif

            collisions.push_back(std::move(collision));
        }
    }

    return collisions;
}

void World::solveCollisions(const CollisionList& collisions)
{
    Solver::resolveCollisions(registry_, collisions);

    for (const auto& col : collisions)
    {
        collisionSignal_.publish(col);
    }
}

#endif

} // namespace ngn
