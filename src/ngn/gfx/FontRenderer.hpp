// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once


#include "Macros.hpp"
namespace ngn {

class FontCollection;
class SpriteRenderer;

class FontRenderer
{
public:
    FontRenderer(SpriteRenderer* spriteRenderer);
    ~FontRenderer();

    void setFontCollection(FontCollection* fontCollection);

    void drawText(uint32_t font, std::string_view text, uint32_t x, uint32_t y);

private:
    SpriteRenderer* spriteRenderer_;
    FontCollection* fontCollection_;
    uint32_t fontIndex_;

    NGN_DISABLE_COPY_MOVE(FontRenderer)
};

} // namespace ngn
