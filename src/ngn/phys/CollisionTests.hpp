// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

class Capsule;
class Circle;
class Collision;
class Line;
class Shape;

std::pair<glm::vec2, glm::vec2> intersections(const glm::vec2& lineStart, const glm::vec2& lineEnd,
                                              const glm::vec2& circleCenter, float circleRadius);

bool testCollision(Collision& collision, const Circle& lhs, const Shape& rhs);
bool testCollision(Collision& collision, const Line& lhs, const Shape& rhs);
bool testCollision(Collision& collision, const Capsule& lhs, const Shape& rhs);
bool testCollision(Collision& collision, const Shape& lhs, const Shape& rhs);

} // namespace ngn
