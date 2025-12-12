// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "Math.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

namespace ngn {

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
