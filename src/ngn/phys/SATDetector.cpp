// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "SATDetector.hpp"

#include <glm/gtx/norm.hpp>
#include <cassert>

namespace ngn {

namespace {

constexpr float LINE_WIDTH = 0.02f;

} // namespace

void SATDetector::testCollision(Collision& collision, const Shape& lhs, const Shape& rhs)
{
    auto test = [&collision, &rhs]<typename T>(const T& lhsT)
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
        }
    };

    switch (lhs.type)
    {
        using enum Shape::Type;

        case Circle:
            test(lhs.circle);
            break;

        case Line:
            test(lhs.line);
            break;

        case Capsule:
            test(lhs.capsule);
            break;
    }
}

void SATDetector::testCollision(Collision& collision, const Circle& lhs, const Circle& rhs)
{
    const auto c2c = rhs.center - lhs.center;
    const auto dist = glm::length(c2c);
    const auto diff = (lhs.radius + rhs.radius) - dist;

    collision.point = lhs.center + c2c / dist * lhs.radius;
    collision.direction = c2c / dist;
    collision.penetration = diff;
    collision.colliding = diff > 0.0f;
}

void SATDetector::testCollision(Collision& collision, const Line& lhs, const Circle& rhs)
{
    testCollision(collision, rhs, lhs.start, lhs.end, LINE_WIDTH);
    collision.direction = -collision.direction;
}

void SATDetector::testCollision(Collision& collision, const Capsule& lhs, const Circle& rhs)
{
    testCollision(collision, rhs, lhs.start, lhs.end, lhs.radius);
    collision.direction = -collision.direction;
}

void SATDetector::testCollision(Collision& collision,
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

void SATDetector::testCollision(Collision& collision,
                              const Line& lhs,
                              const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius)
{
    testCollision(collision, lhs.start, lhs.end, LINE_WIDTH, rhsStart, rhsEnd, rhsRadius);
}

void SATDetector::testCollision(Collision& collision,
                              const Capsule& lhs,
                              const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius)
{
    testCollision(collision, lhs.start, lhs.end, lhs.radius, rhsStart, rhsEnd, rhsRadius);
}

void ngn::SATDetector::testCollision(Collision& collision,
                                   const glm::vec2& lhsStart, const glm::vec2& lhsEnd, float lhsRadius,
                                   const glm::vec2& rhsStart, const glm::vec2& rhsEnd, float rhsRadius)
{
    const auto ab = lhsEnd - lhsStart;
    const auto ac = rhsStart- lhsStart;
    const auto ad = rhsEnd - lhsStart;

    const auto abLen2 = glm::length2(ab);

    const auto acT0 = glm::dot(ab, ac) / abLen2;
    const auto acT = glm::clamp(acT0, 0.0f, 1.0f);

    const auto adT0 = glm::dot(ab, ad) / abLen2;
    const auto adT = glm::clamp(adT0, 0.0f, 1.0f);

    const auto closestC = lhsStart + ab + acT;
    const auto closestD = lhsEnd + ab + adT;

    const auto closestC2C = rhsStart - closestC;
    const auto closestD2D = rhsEnd - closestD;

    const auto distC2 = glm::dot(closestC, closestC);
    const auto distD2 = glm::dot(closestD, closestD);

    if (distC2 <= distD2)
    {
        const auto dist = glm::sqrt(distC2);
        const auto diff = (lhsRadius + rhsRadius) - dist;

        collision.point = closestC + closestC2C / dist * lhsRadius;
        collision.direction = closestC2C / dist;
        collision.penetration = diff;
        collision.colliding = diff > 0.0f;
    }
    else
    {
        const auto dist = glm::sqrt(distD2);
        const auto diff = (lhsRadius + rhsRadius) - dist;

        collision.point = closestD + closestD2D / dist * lhsRadius;
        collision.direction = closestD2D / dist;
        collision.penetration = diff;
        collision.colliding = diff > 0.0f;
    }
}


} // namespace ngn
