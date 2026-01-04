// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Allocators.hpp"
#include <entt/fwd.hpp>
#include <unordered_set>

namespace ngn {

class CollisionPair
{
public:
    entt::entity bodyA{};
    entt::entity bodyB{};

    friend bool operator==(const CollisionPair& lhs, const CollisionPair& rhs)
    {
        return (lhs.bodyA == rhs.bodyA && lhs.bodyB == rhs.bodyB) ||
                (lhs.bodyA == rhs.bodyB && lhs.bodyB == rhs.bodyA);
    }

    bool contains(entt::entity bodyId) const
    {
        return (bodyA == bodyId || bodyB == bodyId);
    }
};

class Collision
{
public:
    CollisionPair pair;
    glm::vec2 point{};
    glm::vec2 direction{};
    float penetration{};
    bool colliding{false};
};

using MovedList = std::vector<uint32_t, LinearAllocator<uint32_t>>;
using CollisionPairSet = std::unordered_set<CollisionPair, std::hash<CollisionPair>,
                                            std::equal_to<>, LinearAllocator<CollisionPair>>;
using CollisionList = std::vector<Collision, LinearAllocator<Collision>>;

} // namespace ngn

namespace std {

template<>
struct hash<ngn::CollisionPair>
{
    std::size_t operator()(const ngn::CollisionPair& cp) const
    {
        using std::hash;
        return hash<entt::entity>{}(cp.bodyA) ^ hash<entt::entity>{}(cp.bodyB);
    }
};

} // namespace
