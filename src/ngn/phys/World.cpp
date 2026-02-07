// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "World.hpp"

#include "Application.hpp"
#include "CommonComponents.hpp"
#include "Instrumentation.hpp"
#include "Functions.hpp"
#include "Math.hpp"
#include "PhysComponents.hpp"
#include "CollisionTests.hpp"
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
        assert(!createInfo.fastMoving || shape.type == Shape::Type::Circle);

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
        .fastMoving = createInfo.fastMoving,
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
    resolveCollisions(registry_, collisions);
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

    auto linForces = registry_->view<
            LinearVelocity,
            Position,
            LastPosition,
            AngularVelocity,
            Rotation,
            const Body,
            ActiveTag>();
    for (auto [e, linVelocity, position, lastPosition, angVelocity, rotation, body] : linForces.each())
    {
        auto* linForce = registry_->try_get<LinearForce>(e);
        if (linForce)
        {
            // add world forces
            linForce->value += config_.gravity;

            // integrate linear velocity
            const auto linVelocityLen2 = glm::length2(linVelocity.value);
            if (linVelocityLen2 > 100.f)
            {
                const auto linVelocityLen = glm::sqrt(linVelocityLen2);
                const auto resistance = 0.5f * linVelocityLen2 * config_.linearDamping * body.friction * 0.5f;
                linForce->value += -linVelocity.value / linVelocityLen * resistance;
            }
            else if (nearZero(linForce->value))
            {
                linVelocity.value = {};
            }

            linVelocity.value += linForce->value * deltaTime;

            linForce->value = {};
        }

        // integrate position
        const auto newPosition = position.value + linVelocity.value * deltaTime;
        if (newPosition != position.value)
        {
            lastPosition.value = position.value;
            position.value = newPosition;

            registry_->emplace_or_replace<TransformChangedTag>(e);
        }


        auto* angForce = registry_->try_get<AngularForce>(e);
        if (angForce)
        {
            // integrate angular velocity
            const auto angVelocityLen2 = angVelocity.value * angVelocity.value;
            if (angVelocityLen2 > 2.f)
            {
                const auto resistance = 0.5f * angVelocityLen2 * config_.angularDamping * body.friction * 10.0f;
                angForce->value += -glm::sign(angVelocity.value) * resistance;
            }
            else if (nearZero(angForce->value))
            {
                angVelocity.value = {};
            }

            angVelocity.value += angForce->value * deltaTime;

            angForce->value = 0.f;
        }

        // integrate rotation
        const auto newRotation = rotation.angle + angVelocity.value * deltaTime;
        if (newRotation != rotation.angle)
        {
            rotation.angle = newRotation;
            rotation.update();

            registry_->emplace_or_replace<TransformChangedTag>(e);
        }
    }
}

MovedList World::updateTree()
{
    MovedList moved{app_->createFrameAllocator<uint32_t>()};

    auto view = registry_->view<
            const Position,
            const Rotation,
            const Scale,
            const Body,
            Shape,
            NodeInfo,
            ActiveTag,
            TransformChangedTag>();
    for (auto [e, pos, rot, sca, body, shape, nodeInfo] : view.each())
    {
        if (body.fastMoving)
        {
            const auto& vel = registry_->get<LinearVelocity>(e);
            const auto tCircle = transform(shape, pos, rot, sca).circle;
            shape = Shape{Capsule{
                .start = tCircle.center,
                .end = tCircle.center + vel.value,
                .radius = tCircle.radius,
            }};
        }
        else
        {
            shape = transform(nodeInfo.origShape, pos, rot, sca);
        }

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

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        auto callback = [&collisionPairs, &node, this](entt::entity entity, const AABB& aabb)
#else
        auto callback = [&collisionPairs, &node](entt::entity entity, const AABB& aabb)
#endif
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
        // HINT
        // Fast moving bodies would need a special handling of collision
        // detection and solving. But since actually only shots are fast
        // moving, correct solving is not needed as they hit something.

        const auto& shapeA = registry_->get<Shape>(col.bodyA);
        const auto& shapeB = registry_->get<Shape>(col.bodyB);

        Collision collision{.pair = col};
        testCollision(collision, shapeA, shapeB);

        if (collision.colliding)
        {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
            debugCollisions_.insert_or_assign(col, collision);
#endif

            const auto bodyA = registry_->get<const Body>(col.bodyA);
            const auto bodyB = registry_->get<const Body>(col.bodyB);

            const auto sensor = bodyA.sensor || bodyB.sensor;

            collisionSignal_.publish(collision, sensor);

            if (!sensor)
                collisions.push_back(std::move(collision));
        }
    }

    return collisions;
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
            return true;
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
