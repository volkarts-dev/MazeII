// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include <glm/glm.hpp>

namespace ngn {

class Sprite
{
public:
    glm::vec4 color{1.0, 1.0, 1.0, 1.0};
    glm::vec4 texCoords{};
    glm::vec2 size{1, 1};
    uint32_t texture{};
};

} // namespace ngn
