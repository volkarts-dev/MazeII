// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Shapes.hpp"
#include "phys/Collision.hpp"
#include <entt/entt.hpp>

namespace ngn {

class Application;
class DynamicTree;

class BodyCreateInfo
{
public:
    float invMass{1.f};
    float friction{0.f};
    float restitution{1.f};
    bool dynamic{true};
};

class WorldConfig
{
public:
    float linearDamping{0.07f};
    float angularDamping{1.5f};
    glm::vec2 gravity{};
};

class World
{
public:
    World(Application* app);
    ~World();

    void setConfig(WorldConfig config);

    template<auto Callback>
    entt::connection addCollisionListener();
    template<auto Callback, typename Type>
    entt::connection addCollisionListener(Type& arg);

    void createBody(entt::entity entity, const BodyCreateInfo& createInfo, Shape shape);

    void update(float deltaTime);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    void debugDrawState(class DebugRenderer* debugRenderer, bool shapes, bool boundingBoxes, bool tree, bool collisions);
#endif

private:
    void integrate(float deltaTime);
    MovedList updateTree();
    CollisionPairSet findPossibleCollisions(const MovedList& moved);
    CollisionList findActualCollsions(const CollisionPairSet& collisionPairs);
    void solveCollisions(const CollisionList& collisions);

private:
    Application* app_;
    entt::registry* registry_;
    DynamicTree* dynamicTree_;

    WorldConfig config_;

    entt::sigh<void(const Collision&)> collisionSignal_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    struct AABBPair
    {
        AABB aabb1;
        AABB aabb2;
    };
    std::unordered_map<CollisionPair, AABBPair> debugPossibleCollisions_;
    std::unordered_map<CollisionPair, Collision> debugCollisions_;
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
