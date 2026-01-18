// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <glm/glm.hpp>

namespace ngn {

class ActiveTag
{
};

class Position
{
public:
    glm::vec2 value{};
};

class Rotation
{
public:
    glm::vec2 dir{0, 1};
    float angle{};

    inline void update()
    {
        dir = {glm::sin(angle), glm::cos(angle)};
    }
};

class Scale
{
public:
    glm::vec2 value{1, 1};
};

} // namespace
