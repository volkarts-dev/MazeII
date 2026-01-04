// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Math.hpp"

#include <glm/gtx/norm.hpp>

namespace ngn {

float atan2(float y, float x)
{
    auto theta = glm::atan(y, x);
    if (theta < 0.f)
        theta += TwoPI;
    return theta;
}

bool nearZero(float value, float e)
{
    const auto abs = glm::abs(value);
    return abs <= e * abs;
}

bool nearZero(glm::vec2 value, float e)
{
    const auto abs = glm::length2(value);
    return abs <= e * abs;
}

} // namespace ngn
