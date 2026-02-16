// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "CollisionTests.hpp"

#include "Collision.hpp"
#include "Math.hpp"
#include "Shapes.hpp"
#include <glm/gtx/norm.hpp>
#include <cassert>

namespace ngn {

namespace {

constexpr float LINE_WIDTH = 0.02f;

void testCollision(Collision& collision, const Circle& lhs, const Circle& rhs)
{
    const auto c2c = rhs.center - lhs.center;
    const auto dist = glm::length(c2c);
    const auto diff = (lhs.radius + rhs.radius) - dist;

    collision.point = lhs.center + c2c / dist * lhs.radius;
    collision.direction = c2c / dist;
    collision.penetration = diff;
    collision.colliding = diff > 0.0f;
}

void testCollision(Collision& collision,
                   const Circle& lhs,
                   const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius)
{
    const auto ab = rhsEnd - rhsStart;
    const auto ac = lhs.center - rhsStart;

    const auto t0 = glm::dot(ab, ac) / glm::length2(ab);
    const auto t = glm::clamp(t0, 0.0f, 1.0f);

    const auto closest = rhsStart + ab * t;

    const auto l2c = closest - lhs.center;
    const auto dist = glm::length(l2c);
    const auto diff = (rhsRadius + lhs.radius) - dist;

    collision.point = lhs.center + l2c / dist * lhs.radius;
    collision.direction = l2c / dist;
    collision.penetration = diff;
    collision.colliding = diff > 0.0f;
}

void testCollision(Collision& collision,
                   const glm::vec2& lhsStart, const glm::vec2& lhsEnd, float lhsRadius,
                   const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius)
{
    const auto d1 = lhsEnd - lhsStart;
    const auto d2 = rhsEnd - rhsStart;
    const auto r = lhsStart - rhsStart;

    const auto a = glm::length2(d1);
    const auto e = glm::length2(d2);
    const auto f = glm::dot(d2, r);
    const auto c = glm::dot(d1, r);
    const auto b = glm::dot(d1, d2);

    // assume that both lines do not degenerate into points (length == 0)

    const auto denom = a * e - b * b;

    auto s = !math::nearZero(denom) ? glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
    auto t = (b * s + f) / e;

    if (t < 0.0f)
    {
        t = 0.0f;
        s = glm::clamp(-c / a, 0.0f, 1.0f);
    }
    else if (t > 1.0f)
    {
        t = 1.0f;
        s = glm::clamp((b - c) / a, 0.0f, 1.0f);
    }

    const auto c1 = lhsStart + d1 * s;
    const auto c2 = rhsStart + d2 * t;

    const auto shortest = c2 - c1;

    const auto dist = glm::length(shortest);
    const auto radii = lhsRadius + rhsRadius;

    const auto diff = radii - dist;

    collision.direction = shortest / dist;
    collision.point = c1 + collision.direction * lhsRadius;
    collision.penetration = diff;
    collision.colliding = diff > 0.0f;
}

inline void testCollision(Collision& collision, const Line& lhs, const Circle& rhs)
{
    testCollision(collision, rhs, lhs.start, lhs.end, LINE_WIDTH);
    collision.direction = -collision.direction;
}

inline void testCollision(Collision& collision, const Capsule& lhs, const Circle& rhs)
{
    testCollision(collision, rhs, lhs.start, lhs.end, lhs.radius);
    collision.direction = -collision.direction;
}

inline void testCollision(Collision& collision,
                   const Line& lhs,
                   const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius)
{
    testCollision(collision, lhs.start, lhs.end, LINE_WIDTH, rhsStart, rhsEnd, rhsRadius);
}

inline void testCollision(Collision& collision,
                   const Capsule& lhs,
                   const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius)
{
    testCollision(collision, lhs.start, lhs.end, lhs.radius, rhsStart, rhsEnd, rhsRadius);
}

template<typename ShapeT>
inline void testCollisionT(Collision& collision, const ShapeT& lhsT, const Shape& rhs)
{
    switch (rhs.type)
    {
        using enum Shape::Type;

        case Circle:
            testCollision(collision, lhsT, rhs.circle);
            break;

        case Line:
            testCollision(collision, lhsT, rhs.line.start, rhs.line.end, LINE_WIDTH);
            break;

        case Capsule:
            testCollision(collision, lhsT, rhs.capsule.start, rhs.capsule.end, rhs.capsule.radius);
            break;

        case Invalid:
            break;
    }
};

} // namespace

// *********************************************************************************************************************

std::pair<glm::vec2, glm::vec2> intersections(const glm::vec2& lineStart, const glm::vec2& lineEnd,
                                              const glm::vec2& circleCenter, float circleRadius)
{
    const auto d = lineEnd - lineStart;
    const auto f = lineStart - circleCenter;

    const auto a = glm::dot(d, d);
    const auto b = 2.0f * glm::dot(d, f);
    const auto c = glm::dot(f, f) - circleRadius * circleRadius;

    const auto det = b * b - 4 * a * c;

    glm::vec2 result[2] = {{NAN, NAN}, {NAN, NAN}};

    if (det > -math::Epsilon)
    {
        const auto sqrtDet = glm::sqrt(det);
        const auto t1 = (-b + sqrtDet) / (2.0f * a);
        const auto t2 = (-b - sqrtDet) / (2.0f * a);

        uint32_t index = 0;

        if (t1 >= 0.0f && t1 <= 1.0f)
            result[index++] = lineStart + d * t1;

        if (det > math::Epsilon && t2 >= 0.0f && t2 <= 1.0f)
            result[index++] = lineStart + d * t2;
    }

    return std::make_pair(result[0], result[1]);
}

// *********************************************************************************************************************

bool testCollision(Collision& collision, const Circle& lhs, const Shape& rhs)
{
    testCollisionT(collision, lhs, rhs);
    return collision.colliding;
}

bool testCollision(Collision& collision, const Line& lhs, const Shape& rhs)
{
    testCollisionT(collision, lhs, rhs);
    return collision.colliding;
}

bool testCollision(Collision& collision, const Capsule& lhs, const Shape& rhs)
{
    testCollisionT(collision, lhs, rhs);
    return collision.colliding;
}

bool testCollision(Collision& collision, const Shape& lhs, const Shape& rhs)
{
    switch (lhs.type)
    {
        using enum Shape::Type;

        case Circle:
            testCollisionT(collision, lhs.circle, rhs);
            break;

        case Line:
            testCollisionT(collision, lhs.line, rhs);
            break;

        case Capsule:
            testCollisionT(collision, lhs.capsule, rhs);
            break;

        case Invalid:
            break;
    }

    return collision.colliding;
}

} // namespace ngn
