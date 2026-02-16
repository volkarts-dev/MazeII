// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once


namespace ngn {

class AABB;
class Line;
class Shape;

bool intersects(const AABB& lhs, const AABB& rhs);
bool intersects(const Line& lhs, const AABB& rhs);

bool intersects(const Line& lhs, const Shape& rhs);

} // namespace ngn
