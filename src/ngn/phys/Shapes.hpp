// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <glm/glm.hpp>

namespace ngn {

class AABB
{
public:
    glm::vec2 topLeft{};
    glm::vec2 bottomRight{};

    inline auto width() const { return bottomRight.x - topLeft.x; }
    inline auto height() const { return bottomRight.y - topLeft.y; }

    inline void extend(glm::vec2 extent)
    {
        topLeft -= extent / 2.f;
        bottomRight += extent / 2.f;
    }
};

class Circle
{
public:
    glm::vec2 center{};
    float radius{};
};

class Line
{
public:
    glm::vec2 start{};
    glm::vec2 end{};
};

class Capsule
{
public:
    glm::vec2 start{};
    glm::vec2 end{};
    float radius{};
};

class Shape
{
public:
    enum class Type : uint32_t
    {
        Circle,
        Line,
        Capsule,
    };

public:
    Shape(Circle&& c);
    Shape(Line&& l);
    Shape(Capsule&& c);

    Type type;

    union
    {
        Circle circle;
        Line line;
        Capsule capsule;
    };
};

} // namespace ngn
