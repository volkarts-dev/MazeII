// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Collision.hpp"

namespace ngn {

void resolveCollisions(entt::registry* registry, const CollisionInfoList& collisions);
void resolveCollision(entt::registry* registry, const CollisionInfo& collision);

} // namespace ngn
