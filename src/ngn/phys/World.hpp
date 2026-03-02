// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "DynamicTree.hpp"
#include "Macros.hpp"
#include "Shapes.hpp"
#include "phys/Collision.hpp"
#include <entt/signal/sigh.hpp>

namespace ngn {

class Application;

class BodyCreateInfo
{
public:
    Layers layers{Layers::All};
    float invMass{1.f};
    float restitution{1.f};
    float friction{1.f};
    bool sensor{false};
    bool dynamic{true};
    bool useForce{true};
    bool fastMoving{false};
};

class WorldConfig
{
public:
    float linearDamping{1.0f};
    float angularDamping{1.0f};
    glm::vec2 gravity{};
};

class World
{
public:
    using CollisionCallback = entt::delegate<void(const Collision&)>;

public:
    World(Application* app);
    ~World();

    void setConfig(WorldConfig config);

    template<auto Callback, typename... Args>
    entt::connection addCollisionListener(Args&&... args);

    void createBody(entt::entity entity, const BodyCreateInfo& createInfo, Shape shape);

    void update(float deltaTime);

    template<typename Callback>
    inline void query(const AABB& aabb, Layers layers, const Callback& callback) const
    {
        return dynamicTree_->query(aabb, layers, callback);
    }

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    void debugDrawState(class DebugRenderer* debugRenderer, bool shapes, bool boundingBoxes, bool tree, bool collisions);
#endif

private:
    Shape transformShape(entt::entity entity, const Shape& origShape);
    void updateActive();
    void integrate(float deltaTime);
    MovedList updateTree();
    CollisionPairSet findPossibleCollisions(const MovedList& moved);
    CollisionInfoList findActualCollsions(const CollisionPairSet& collisionPairs);

private:
    Application* app_;
    entt::registry* registry_;
    DynamicTree* dynamicTree_;

    WorldConfig config_;

    entt::sigh<void(const CollisionInfo&, bool sensor)> collisionSignal_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    struct AABBPair
    {
        AABB aabb1;
        AABB aabb2;
    };
    std::unordered_map<CollisionPair, AABBPair> debugPossibleCollisions_;
    std::unordered_map<CollisionPair, Collision> debugCollisions_;
    void removeDebugState(entt::entity entity);
#endif

    NGN_DISABLE_COPY_MOVE(World)
};

template<auto Callback, typename... Args>
inline entt::connection World::addCollisionListener(Args&&... args)
{
    entt::sink s{collisionSignal_};
    return s.connect<Callback>(std::forward<Args>(args)...);
}

} // namespace ngn
