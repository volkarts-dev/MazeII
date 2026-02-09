// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

class AABB;
class Circle;
class Collision;
class Line;
class Shape;

bool intersects(const AABB& lhs, const AABB& rhs);
bool intersects(const Line& lhs, const AABB& rhs);

void testCollision(Collision& collision, const Shape& lhs, const Shape& rhs);

std::pair<glm::vec2, glm::vec2> intersections(const glm::vec2& lineStart, const glm::vec2& lineEnd,
                                              const glm::vec2& circleCenter, float circleRadius);

} // namespace ngn
