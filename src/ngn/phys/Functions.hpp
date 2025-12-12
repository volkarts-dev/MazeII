// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include <glm/fwd.hpp>

namespace ngn {

class AABB;
class Position;
class Rotation;
class Scale;
class Shape;

AABB calculateAABB(const Shape& shape);
bool contains(const AABB& lhs, const AABB& rhs);
bool intersects(const AABB& lhs, const AABB& rhs);
AABB combine(const AABB& one, const AABB& two);
float area(const AABB& aabb);

glm::vec2 transform(glm::vec2 vec, const Position& pos, const Rotation& rot, const Scale& sca);
Shape transform(Shape shape, const Position& pos, const Rotation& rot, const Scale& sca);

} // namespace ngn

