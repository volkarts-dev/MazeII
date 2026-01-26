// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

namespace ngn {

class AABB;
class Intersection;
class Line;

bool intersects(const AABB& lhs, const AABB& rhs);
bool intersects(const Line& lhs, const AABB& rhs);


} // namespace ngn
