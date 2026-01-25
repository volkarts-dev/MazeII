// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "SpriteRenderer.hpp"

namespace ngn {

class FontCollection;

class UiRenderer
{
public:
    UiRenderer(Renderer* renderer, uint32_t batchSize);
    ~UiRenderer();

    void setFontCollection(FontCollection* fontCollection);

    void writeText(uint32_t font, std::string_view text, uint32_t x, uint32_t y);

    void updateView(const glm::mat4& view);
    void updateView(const glm::mat4& view, uint32_t frameIndex);

    void draw(CommandBuffer* commandBuffer);

private:
    SpriteRenderer spriteRenderer_;
    FontCollection* fontCollection_;
    uint32_t fontIndex_;

    NGN_DISABLE_COPY_MOVE(UiRenderer)
};

} // namespace ngn
