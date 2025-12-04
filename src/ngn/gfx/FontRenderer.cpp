// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "FontRenderer.hpp"

#include "FontCollection.hpp"
#include "SpritePipeline.hpp"
#include "SpriteRenderer.hpp"

namespace ngn {

FontRenderer::FontRenderer(SpriteRenderer* spriteRenderer, FontCollection* fontCollection) :
    spriteRenderer_{spriteRenderer},
    fontCollection_{fontCollection}
{
    fontIndex_ = spriteRenderer_->addImages({{fontCollection_->image()}});
}

FontRenderer::~FontRenderer()
{
    delete fontCollection_;
}

void FontRenderer::drawText(uint32_t font, std::string_view text, uint32_t x, uint32_t y)
{
    glm::vec2 pos{x, y};

    for (uint32_t i = 0; i < text.size(); i++)
    {
        const auto& glyph = fontCollection_->glyphInfo(font)[static_cast<uint8_t>(text[i]) - 32];

        spriteRenderer_->renderSprite(SpriteVertex{
            .position = pos + glyph.size / 2.f + glyph.bearing,
            .rotation = 0.0f,
            .scale = glyph.size,
            .color = {1.0, 1.0, 1.0, 1.0},
            .texIndex = fontIndex_,
            .texCoords = glyph.texCoords,
        });

        pos.x += glyph.advance;
    }
}

} // namespace ngn
