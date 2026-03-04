// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "SpriteRenderer.hpp"

namespace ngn {



class UiRenderer : public SpriteRenderer
{
public:
    UiRenderer(Renderer* renderer, uint32_t batchSize);
    ~UiRenderer() override = default;

private:
    NGN_DISABLE_COPY_MOVE(UiRenderer)
};

} // namespace ngn
