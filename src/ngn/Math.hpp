// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <numbers>

namespace ngn::math {

constexpr float Epsilon = 1e-9f;

constexpr float PI = std::numbers::pi_v<float>;
constexpr float TwoPI = std::numbers::pi_v<float> * 2.f;
constexpr float HalfPI = std::numbers::pi_v<float> / 2.f;

inline float atan2(float y, float x)
{
    auto theta = glm::atan(y, x);
    if (theta < 0.f)
        theta += TwoPI;
    return theta;
}

inline bool nearZero(float value, float e = Epsilon)
{
    const auto abs = glm::abs(value);
    return abs <= e * abs;
}

inline bool nearZero(glm::vec2 value, float e = Epsilon)
{
    const auto abs = glm::length2(value);
    return abs <= e * abs;
}

inline float det(const glm::vec2& a, const glm::vec2& b)
{
    return a.x * b.y - a.y * b.x;
}

inline float angleDiff(float a, float b)
{
    const auto diff = glm::abs(a - b);
    return diff > PI ? TwoPI - diff : diff;
}

} // namespace ngn::math
