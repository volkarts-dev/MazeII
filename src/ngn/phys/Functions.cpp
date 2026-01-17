// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Functions.hpp"

#include "Shapes.hpp"
#include "CommonComponents.hpp"

namespace ngn {

AABB calculateAABB(const Shape& shape)
{
    AABB aabb;

    switch (shape.type)
    {
        using enum Shape::Type;

        case Circle:
        {
            auto& c = shape.circle;
            aabb.topLeft = c.center - c.radius;
            aabb.bottomRight = c.center + c.radius;
            break;
        }

        case Capsule:
        {
            auto& c = shape.capsule;
            aabb.topLeft = {glm::min(c.start.x, c.end.x) - c.radius, glm::min(c.start.y, c.end.y) - c.radius};
            aabb.bottomRight = {glm::max(c.start.x, c.end.x) + c.radius, glm::max(c.start.y, c.end.y) + c.radius};
            break;
        }

        case Line:
        {
            auto& l = shape.line;
            aabb.topLeft = {glm::min(l.start.x, l.end.x), glm::min(l.start.y, l.end.y)};
            aabb.bottomRight = {glm::max(l.start.x, l.end.x), glm::max(l.start.y, l.end.y)};
            break;
        }
    }

    return aabb;
}

bool intersects(const AABB& lhs, const AABB& rhs)
{
    return !(
            lhs.bottomRight.x < rhs.topLeft.x ||
            lhs.bottomRight.y < rhs.topLeft.y ||
            rhs.bottomRight.x < lhs.topLeft.x ||
            rhs.bottomRight.y < lhs.topLeft.y
            );
}

bool intersects(const Line& lhs, const AABB& rhs)
{
    AABB test = rhs;

    const auto invD = 1.0f / (lhs.end - lhs.start);
    if (invD.x < 0)
        std::swap(test.topLeft.x, test.bottomRight.x);
    if (invD.y < 0)
        std::swap(test.topLeft.y, test.bottomRight.y);

    const auto near = (test.topLeft - lhs.start) * invD;
    const auto far = (test.bottomRight - lhs.start) * invD;

    if (near.x > far.x && near.y > far.y)
        return false;

    const auto nearT = near.x > near.y ? near.x : near.y;
    const auto farT = far.x < far.y ? far.x : far.y;

    if (nearT > 1.0f || farT < 0.0f)
        return false;

    return true;
}

bool contains(const AABB& lhs, const AABB& rhs)
{
    return
            rhs.topLeft.x >= lhs.topLeft.x &&
            rhs.topLeft.y >= lhs.topLeft.y &&
            rhs.bottomRight.x <= lhs.bottomRight.x &&
            rhs.bottomRight.y <= lhs.bottomRight.y;
}

AABB combine(const AABB& one, const AABB& two)
{
    return {
        .topLeft = {
            glm::min(one.topLeft.x, two.topLeft.x),
            glm::min(one.topLeft.y, two.topLeft.y),
        },
        .bottomRight = {
            glm::max(one.bottomRight.x, two.bottomRight.x),
            glm::max(one.bottomRight.y, two.bottomRight.y),
        },
    };
}

float area(const AABB& aabb)
{
    return (aabb.bottomRight.x - aabb.topLeft.x) * (aabb.bottomRight.y - aabb.topLeft.y);
}

glm::vec2 rotate(const glm::vec2& vec, const glm::vec2& dir)
{
    return {
         vec.x * dir.y + vec.y * dir.x,
        -vec.x * dir.x + vec.y * dir.y,
    };
}

glm::vec2 transform(glm::vec2 vec, const Position& pos, const Rotation& rot, const Scale& sca)
{
    vec *= sca.value;
    vec = rotate(vec, rot.dir);
    vec += pos.value;
    return vec;
}

Shape transform(Shape shape, const Position& pos, const Rotation& rot, const Scale& sca)
{
    switch (shape.type)
    {
        using enum Shape::Type;

        case Circle:
        {
            auto& c = shape.circle;
            c.center = transform(c.center, pos, rot, sca);
            c.radius *= std::max(sca.value.x, sca.value.y);
            break;
        }

        case Capsule:
        {
            auto& c = shape.capsule;
            c.start = transform(c.start, pos, rot, sca);
            c.end= transform(c.end, pos, rot, sca);
            break;
        }

        case Line:
        {
            auto& l = shape.line;
            l.start = transform(l.start, pos, rot, sca);
            l.end= transform(l.end, pos, rot, sca);
            break;
        }
    }

    return shape;
}

} // namespace ngn
