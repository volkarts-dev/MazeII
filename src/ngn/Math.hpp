// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <glm/fwd.hpp>
#include <numbers>

namespace ngn {

constexpr float Epsilon = 1e-6f;

constexpr float PI = std::numbers::pi_v<float>;
constexpr float TwoPI = std::numbers::pi_v<float> * 2.f;
constexpr float HalfPI = std::numbers::pi_v<float> / 2.f;

float atan2(float y, float x);
bool nearZero(float value, float e = Epsilon);
bool nearZero(glm::vec2 value, float e = Epsilon);

} // namespace ngn
