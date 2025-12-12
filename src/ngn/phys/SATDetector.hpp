// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Collision.hpp"
#include "Shapes.hpp"

namespace ngn {

class SATDetector
{
public:
    static void testCollision(Collision& collision, const Shape& lhs, const Shape& rhs);

private:
    static void testCollision(Collision& collision,
                              const Circle& lhs,
                              const Circle& rhs);

    static void testCollision(Collision& collision,
                              const Line& lhs,
                              const Circle& rhs);

    static void testCollision(Collision& collision,
                              const Capsule& lhs,
                              const Circle& rhs);

    static void testCollision(Collision& collision,
                              const Circle& lhs,
                              const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius);

    static void testCollision(Collision& collision,
                              const Line& lhs,
                              const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius);

    static void testCollision(Collision& collision,
                              const Capsule& lhs,
                              const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius);

    static void testCollision(Collision& collision,
                              const glm::vec2& lhsStart, const glm::vec2& lhsEnd, float lhsRadius,
                              const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius);
private:
    SATDetector() = delete;
};

} // namespace ngn
