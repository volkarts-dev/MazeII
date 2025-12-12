// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Shapes.hpp"
#include "phys/Collision.hpp"
#include <entt/entt.hpp>

namespace ngn {

class DynamicTree;

class BodyCreateInfo
{
public:
    float invMass{1.f};
    float friction{0.f};
    float restitution{1.f};
};

class World
{
public:
    World(entt::registry* registry);
    ~World();

    template<auto Callback>
    entt::connection addCollisionListener();
    template<auto Callback, typename Type>
    entt::connection addCollisionListener(Type& arg);

    void createBody(entt::entity entity, const BodyCreateInfo& createInfo, Shape shape);

    void update(float deltaTime);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    void debugDrawState(class DebugRenderer* debugRenderer);
#endif

private:
    void integrate(float deltaTime);
    MovedList updateTree();
    CollisionPairList findPossibleCollisions(const MovedList& moved);
    CollisionList findActualCollsions(const CollisionPairList& collisionPairs);
    void solveCollisions(const CollisionList& collisions);

private:
    entt::registry* registry_;
    DynamicTree* dynamicTree_;

    float linearDamping_;
    float angularDamping_;
    glm::vec2 gravity_;

    entt::sigh<void(const Collision&)> collisionSignal_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    std::unordered_map<entt::entity, AABB> debugPossibleCollisions_;
    std::unordered_map<entt::entity, Collision> debugCollisions_;
#endif

    NGN_DISABLE_COPY_MOVE(World)
};

template<auto Callback, typename Type>
inline entt::connection World::addCollisionListener(Type& arg)
{
    entt::sink s{collisionSignal_};
    return s.connect<Callback>(arg);
}

template<auto Callback>
inline entt::connection World::addCollisionListener()
{
    entt::sink s{collisionSignal_};
    return s.connect<Callback>();
}

} // namespace ngn
