// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Functions.hpp"

#include "Shapes.hpp"
#include "CommonComponents.hpp"

namespace ngn {

namespace {

template<typename... Trans>
inline Shape transformIntern(Shape shape, Trans&&... trans)
{
    switch (shape.type)
    {
        using enum Shape::Type;

        case Circle:
        {
            auto& c = shape.circle;
            c.center = transform(c.center, std::forward<Trans>(trans)...);
            if constexpr (sizeof...(Trans) >= 3)
            {
                const auto args = std::make_tuple(std::forward<Trans>(trans)...);
                const auto& sca = std::get<2>(args);
                c.radius *= std::max(sca.value.x, sca.value.y);
            }
            break;
        }

        case Capsule:
        {
            auto& c = shape.capsule;
            c.start = transform(c.start, std::forward<Trans>(trans)...);
            c.end= transform(c.end, std::forward<Trans>(trans)...);
            break;
        }

        case Line:
        {
            auto& l = shape.line;
            l.start = transform(l.start, std::forward<Trans>(trans)...);
            l.end= transform(l.end, std::forward<Trans>(trans)...);
            break;
        }

        case Invalid:
            break;
    }

    return shape;
}

} // namespace

AABB calculateAABB(const Circle& circle)
{
    return {
        .topLeft = circle.center - circle.radius,
        .bottomRight = circle.center + circle.radius,
    };
}

AABB calculateAABB(const Capsule& capsule)
{
    return {
        .topLeft = glm::min(capsule.start, capsule.end) - capsule.radius,
        .bottomRight = glm::max(capsule.start, capsule.end) + capsule.radius,
    };
}

AABB calculateAABB(const Line& line)
{
    return {
        .topLeft = glm::min(line.start, line.end),
        .bottomRight = glm::max(line.start, line.end),
    };
}

AABB calculateAABB(const Shape& shape)
{
    AABB aabb;

    switch (shape.type)
    {
        using enum Shape::Type;

        case Circle:
            return calculateAABB(shape.circle);

        case Capsule:
            return calculateAABB(shape.capsule);

        case Line:
            return calculateAABB(shape.line);

        case Invalid:
            break;
    }

    return aabb;
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

glm::vec2 transform(glm::vec2 vec, const Position& pos)
{
    vec += pos.value;
    return vec;
}

glm::vec2 transform(glm::vec2 vec, const Position& pos, const Rotation& rot, const Scale& sca)
{
    vec *= sca.value;
    vec = rotate(vec, rot.dir);
    vec += pos.value;
    return vec;
}

Shape transform(Shape shape, const Position& pos)
{
    return transformIntern(std::move(shape), pos);
}

Shape transform(Shape shape, const Position& pos, const Rotation& rot, const Scale& sca)
{
    return transformIntern(std::move(shape), pos, rot, sca);
}

} // namespace ngn
