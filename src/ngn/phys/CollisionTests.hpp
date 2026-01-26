// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Collision.hpp"
#include "Shapes.hpp"

namespace ngn {

void testCollision(Collision& collision, const Shape& lhs, const Shape& rhs);

// minDistance(const Shape& lhs, const Shape& rhs);

} // namespace ngn
