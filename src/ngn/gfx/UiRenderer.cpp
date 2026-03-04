// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "UiRenderer.hpp"

namespace ngn {

UiRenderer::UiRenderer(Renderer* renderer, uint32_t batchSize) :
    SpriteRenderer{renderer, batchSize}
{
}

} // namespace ngn
