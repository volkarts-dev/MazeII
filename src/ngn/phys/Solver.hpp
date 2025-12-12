// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "Collision.hpp"

namespace ngn {

class Solver
{
public:
    static void resolveCollisions(entt::registry* registry, const CollisionList& collisions);

private:
    static void resolveCollision(entt::registry* registry, const Collision& collision);

    Solver() = delete;
};

} // namespace ngn

