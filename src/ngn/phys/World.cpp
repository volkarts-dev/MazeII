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

World::World(entt::registry* registry) :
    registry_{registry},
    dynamicTree_{new DynamicTree{registry_}},
    linearDamping_{0.07f},
    angularDamping_{1.5f},
    gravity_{}
{
}

World::~World()
{
    delete dynamicTree_;
}

void World::createBody(entt::entity entity, const BodyCreateInfo& createInfo, Shape shape)
{
    registry_->emplace<LinearVelocity>(entity);
    registry_->emplace<LinearForce>(entity);
    registry_->emplace<AngularVelocity>(entity);
    registry_->emplace<AngularForce>(entity);

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

void World::debugDrawState(DebugRenderer* debugRenderer)
{
    auto drawShape = [](DebugRenderer* renderer, const Shape& shape, const glm::vec4& color)
    {
        switch (shape.type)
        {
            using enum Shape::Type;

            case Circle:
            {
                renderer->drawCircle(shape.circle.center, shape.circle.radius, color);
                break;
            }

            case Line:
            {
                renderer->drawLine(shape.line.start, shape.line.end, color);
                break;
            }

            case Capsule:
            {
                break;
            }
        }
    };

    auto fillShape = [](DebugRenderer* renderer, const Shape& shape, const glm::vec4& color)
    {
        switch (shape.type)
        {
            using enum Shape::Type;

            case Circle:
            {
                renderer->fillCircle(shape.circle.center, shape.circle.radius, color);
                break;
            }

            case Line:
            {
                renderer->drawLine(shape.line.start, shape.line.end, color);
                break;
            }

            case Capsule:
            {
                break;
            }
        }
    };

    for (auto [e, shape]: registry_->view<Shape>().each())
    {
        fillShape(debugRenderer, shape, {0, 1, 0, 0.1});
    }

    dynamicTree_->walkTree([debugRenderer](const TreeNode& node)
    {
        debugRenderer->drawAABB(node.aabb.topLeft, node.aabb.bottomRight, {1, 0, 1, 0.3});
    });

    for (const auto& col : debugPossibleCollisions_)
    {
        debugRenderer->drawAABB(col.second.topLeft, col.second.bottomRight, {1, 1, 0, 0.6});
    }

    for (const auto& col : debugCollisions_)
    {
        drawShape(debugRenderer, registry_->get<Shape>(col.first), {1, 0, 0, 0.9});
        const auto start = col.second.point;
        const auto end = start + col.second.direction * col.second.penetration;
        debugRenderer->drawCircle(start, 2.f, {1, 0, 0, 0.9});
        debugRenderer->drawLine(start, end, {1, 0, 0, 0.9});
    }
}

void World::integrate(float deltaTime)
{
    auto linForces = registry_->view<LinearForce, LinearVelocity>();
    for (auto [e, force, velocity] : linForces.each())
    {
        force.value += gravity_;

        const auto veloLen2 = glm::length2(velocity.value);
        if (veloLen2 > 100.f)
        {
            const auto resistance = -(velocity.value / glm::sqrt(veloLen2)) * veloLen2;
            force.value += resistance * linearDamping_;
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
            force.value += resistance * angularDamping_;
        }
        else if (nearZero(force.value))
        {
            velocity.value = {};
        }

        velocity.value += force.value * deltaTime;
        force.value = 0.f;
    }

    auto linVelocities = registry_->view<LinearVelocity, Position>();
    for (auto [e, velocity, position] : linVelocities.each())
    {
        position.value += velocity.value * deltaTime;
    }

    auto angVelocities = registry_->view<AngularVelocity, Rotation>();
    for (auto [e, velocity, rotation] : angVelocities.each())
    {
        rotation.angle += velocity.value * deltaTime;
        rotation.update();
    }
}

MovedList World::updateTree()
{
    MovedList moved{Application::get()->frameAllocator()};

    for (auto [e, pos, rot, sca, shape, nodeInfo] : registry_->view<Position, Rotation, Scale, Shape, NodeInfo>().each())
    {
        shape = transform(nodeInfo.origShape, pos, rot, sca);
        auto aabb = calculateAABB(shape);

        const auto upd = dynamicTree_->updateObject(nodeInfo.nodeId, aabb);
        if (upd)
            moved.pushBack(nodeInfo.nodeId);
    }

    return moved;
}

CollisionPairList World::findPossibleCollisions(const MovedList& moved)
{
    CollisionPairList collisionPairs{Application::get()->frameAllocator()};

    for (const auto index : moved)
    {
        const auto& node = dynamicTree_->node(index);
        bool found = false;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        debugPossibleCollisions_.erase(node.entity);
        debugCollisions_.erase(node.entity);
#endif

        dynamicTree_->query(node.aabb, [&collisionPairs, &node, &found](entt::entity entity)
        {
            if (entity != node.entity)
            {
                collisionPairs.pushBack({
                                            .bodyA = node.entity,
                                            .bodyB = entity,
                                        });
                found = true;
            }
            return true;
        });

        if (found)
        {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
            debugPossibleCollisions_.insert(std::make_pair(node.entity, node.aabb));
#endif
        }
    }

    return collisionPairs;
}

CollisionList World::findActualCollsions(const CollisionPairList& collisionPairs)
{
    CollisionList collisions{Application::get()->frameAllocator()};

    for (const auto& col : collisionPairs)
    {
        const auto& shapeA = registry_->get<Shape>(col.bodyA);
        const auto& shapeB = registry_->get<Shape>(col.bodyB);

        Collision collision{.pair = col};
        SATDetector::testCollision(collision, shapeA, shapeB);
        if (collision.colliding)
        {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
            debugCollisions_.insert(std::make_pair(col.bodyA, collision));
#endif

            collisions.pushBack(std::move(collision));
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
