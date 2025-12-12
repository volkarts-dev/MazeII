// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include <glm/fwd.hpp>

namespace ngn {

constexpr float Epsilon = 1e-6f;

bool nearZero(float value, float e = Epsilon);
bool nearZero(glm::vec2 value, float e = Epsilon);

} // namespace ngn
