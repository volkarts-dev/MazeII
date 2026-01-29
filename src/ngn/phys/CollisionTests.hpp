// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

class AABB;
class Collision;
class Line;
class Shape;

bool intersects(const AABB& lhs, const AABB& rhs);
bool intersects(const Line& lhs, const AABB& rhs);

void testCollision(Collision& collision, const Shape& lhs, const Shape& rhs);

// minDistance(const Shape& lhs, const Shape& rhs);

} // namespace ngn
