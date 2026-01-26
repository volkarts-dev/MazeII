// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "World.hpp"

#include "Application.hpp"
#include "CommonComponents.hpp"
#include "Instrumentation.hpp"
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

class LastPosition
{
public:
    glm::vec2 value;
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
        if (createInfo.useForce)
        {
            registry_->emplace<LinearForce>(entity);
            registry_->emplace<AngularForce>(entity);
        }
        registry_->emplace<LinearVelocity>(entity);
        registry_->emplace<AngularVelocity>(entity);
    }

    if (const auto* pos = registry_->try_get<Position>(entity); !pos)
        registry_->emplace<Position>(entity, glm::vec2{});

    registry_->emplace<LastPosition>(entity);
    registry_->emplace<TransformChangedTag>(entity);

    registry_->emplace<Body>(entity, Body{
                                 .invMass = createInfo.invMass,
                                 .friction = createInfo.friction,
                                 .restitution = createInfo.restitution,
                                 .sensor = createInfo.sensor,
                             });

    const auto transformedShape = transformShape(entity, shape);
    registry_->emplace<Shape>(entity, transformedShape);

    auto nodeId = InvalidIndex;
    if (registry_->any_of<ActiveTag>(entity))
        nodeId = dynamicTree_->addObject(calculateAABB(transformedShape), entity);
    registry_->emplace<NodeInfo>(entity, shape, nodeId);
}

void World::update(float deltaTime)
{
    updateActive();
    integrate(deltaTime);
    const auto moved = updateTree();
    const auto possibleCollisions = findPossibleCollisions(moved);
    const auto collisions = findActualCollsions(possibleCollisions);
    solveCollisions(collisions);
}

Shape World::transformShape(entt::entity entity, const Shape& origShape)
{
    auto [pos, rot, sca]= registry_->try_get<const Position, const Rotation, const Scale>(entity);
    if (sca)
        return transform(origShape, *pos, *rot, *sca);
    else if (pos)
        return transform(origShape, *pos);
    return origShape;
}

void World::updateActive()
{
    auto view = registry_->view<NodeInfo>();
    for (auto [e, nodeInfo] : view.each())
    {
        const auto active = registry_->any_of<ActiveTag>(e);

        if (active && nodeInfo.nodeId == InvalidIndex)
        {
            auto shape = registry_->get<Shape>(e);
            shape = transformShape(e, nodeInfo.origShape);

            nodeInfo.nodeId = dynamicTree_->addObject(calculateAABB(shape), e);
        }
        else if (!active && nodeInfo.nodeId != InvalidIndex)
        {
            dynamicTree_->removeObject(nodeInfo.nodeId);

            nodeInfo.nodeId = InvalidIndex;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
            removeDebugState(e);
#endif
        }
    }
}

void World::integrate(float deltaTime)
{
    NGN_INSTRUMENT_FUNCTION();

    auto linForces = registry_->view<LinearForce, LinearVelocity, const Body, ActiveTag>();
    for (auto [e, force, velocity, body] : linForces.each())
    {
        force.value += config_.gravity;

        const auto veloLen2 = glm::length2(velocity.value);
        if (veloLen2 > 100.f)
        {
            const auto veloLen = glm::sqrt(veloLen2);
            const auto resistance = 0.5f * veloLen2 * config_.linearDamping * body.friction * 0.5f;
            force.value += -velocity.value / veloLen * resistance;
        }
        else if (nearZero(force.value))
        {
            velocity.value = {};
        }

        velocity.value += force.value * deltaTime;

        force.value = {};
    }

    auto angForces = registry_->view<AngularForce, AngularVelocity, const Body, ActiveTag>();
    for (auto [e, force, velocity, body] : angForces.each())
    {
        const auto veloLen = velocity.value;
        const auto veloLen2 = veloLen * veloLen;
        if (veloLen2 > 2.f)
        {
            const auto resistance = 0.5f * veloLen2 * config_.angularDamping * body.friction * 10.0f;
            force.value += -glm::sign(veloLen) * resistance;
        }
        else if (nearZero(force.value))
        {
            velocity.value = {};
        }

        velocity.value += force.value * deltaTime;
        force.value = 0.f;
    }

    auto linVelocities = registry_->view<LinearVelocity, Position, LastPosition, ActiveTag>();
    for (auto [e, velocity, position, lastPosition] : linVelocities.each())
    {
        const auto newPos = position.value + velocity.value * deltaTime;
        if (newPos != position.value)
        {
            lastPosition.value = position.value;
            position.value = newPos;

            registry_->emplace_or_replace<TransformChangedTag>(e);
        }
    }

    auto angVelocities = app_->registry()->view<AngularVelocity, Rotation, ActiveTag>();
    for (auto [e, velocity, rotation] : angVelocities.each())
    {
        bool changed = false;
        const auto newRot = rotation.angle + velocity.value * deltaTime;
        if (newRot != rotation.angle)
        {
            changed = true;
            rotation.angle = newRot;
        }
        if (changed)
        {
            rotation.update();
            registry_->emplace_or_replace<TransformChangedTag>(e);
        }
    }
}

MovedList World::updateTree()
{
    MovedList moved{app_->createFrameAllocator<uint32_t>()};

    auto view = registry_->view<Position, Rotation, Scale, Body, Shape, NodeInfo, ActiveTag, TransformChangedTag>();
    for (auto [e, pos, rot, sca, body, shape, nodeInfo] : view.each())
    {
        shape = transform(nodeInfo.origShape, pos, rot, sca);

        dynamicTree_->updateObject(nodeInfo.nodeId, calculateAABB(shape));

        // only dynamic bodies can move
        if (registry_->all_of<LinearVelocity>(e))
            moved.push_back(nodeInfo.nodeId);

        registry_->remove<TransformChangedTag>(e);
    }

    return moved;
}

CollisionPairSet World::findPossibleCollisions(const MovedList& moved)
{
    NGN_INSTRUMENT_FUNCTION();

    CollisionPairSet collisionPairs{app_->createFrameAllocator<CollisionPair>()};
    collisionPairs.reserve(moved.size());

    for (const auto index : moved)
    {
        const auto& node = dynamicTree_->node(index);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        removeDebugState(node.entity);
#endif

        auto callback = [&collisionPairs, &node
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
                , this
#endif
                ](entt::entity entity, const AABB& aabb)
        {
            NGN_UNUSED(aabb);

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
    NGN_INSTRUMENT_FUNCTION();

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

            collisionSignal_.publish(collision);

            if (!registry_->get<Body>(col.bodyA).sensor && !registry_->get<Body>(col.bodyB).sensor)
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

            case Invalid:
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

            case Invalid:
                break;
        }
    };

    if (shapes)
    {
        for (auto [e, shape]: registry_->view<ActiveTag, Shape>().each())
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

void World::removeDebugState(entt::entity entity)
{
    for (auto it = debugPossibleCollisions_.begin(); it != debugPossibleCollisions_.end(); )
    {
        if (it->first.contains(entity))
        {
            debugCollisions_.erase(it->first);
            it = debugPossibleCollisions_.erase(it);
        }
        else
        {
            it++;
        }
    }
}

#endif

} // namespace ngn

NGN_INSTRUMENTATION_EPILOG(World)
