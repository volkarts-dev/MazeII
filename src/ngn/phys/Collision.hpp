// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "Allocators.hpp"
#include "util/Vector.hpp"
#include <entt/fwd.hpp>

namespace ngn {

class CollisionPair
{
public:
    entt::entity bodyA{};
    entt::entity bodyB{};
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

using MovedList = Vector<uint32_t, LinearAllocator>;
using CollisionPairList = Vector<CollisionPair, LinearAllocator>;
using CollisionList = Vector<Collision, LinearAllocator>;

} // namespace ngn
