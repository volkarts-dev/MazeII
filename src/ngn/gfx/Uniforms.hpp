// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <glm/glm.hpp>

namespace ngn {

class ViewProjection
{
public:
    glm::mat4 view;
    glm::mat4 proj;
};

} // namespace ngn
