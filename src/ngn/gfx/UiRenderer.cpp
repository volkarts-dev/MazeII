// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "UiRenderer.hpp"

#include "FontCollection.hpp"
#include "SpritePipeline.hpp"
#include "SpriteRenderer.hpp"

namespace ngn {

UiRenderer::UiRenderer(Renderer* renderer, uint32_t batchSize) :
    spriteRenderer_{renderer, batchSize},
    fontCollection_{},
    fontIndex_{InvalidIndex}
{
}

UiRenderer::~UiRenderer()
{
    delete fontCollection_;
}

void UiRenderer::setFontCollection(FontCollection* fontCollection)
{
    fontCollection_ = fontCollection;

    fontIndex_ = spriteRenderer_.addImages({{fontCollection_->image()}});
}

void UiRenderer::writeText(uint32_t font, std::string_view text, uint32_t x, uint32_t y)
{
    glm::vec2 pos{x, y};

    for (uint32_t i = 0; i < text.size(); i++)
    {
        const auto& glyph = fontCollection_->glyphInfo(font)[static_cast<uint8_t>(text[i]) - 32];

        spriteRenderer_.renderSprite(SpriteVertex{
            .position = pos + glyph.size / 2.f + glyph.bearing,
            .rotation = 0.0f,
            .scale = glyph.size,
            .color = {1.0, 1.0, 1.0, 1.0},
            .texCoords = glyph.texCoords,
            .texIndex = fontIndex_,
        });

        pos.x += glyph.advance;
    }
}

void UiRenderer::updateView(const glm::mat4& view)
{
    spriteRenderer_.updateView(view);
}

void UiRenderer::updateView(const glm::mat4& view, uint32_t frameIndex)
{
    spriteRenderer_.updateView(view, frameIndex);
}

void UiRenderer::draw(CommandBuffer* commandBuffer)
{
    spriteRenderer_.draw(commandBuffer);
}

} // namespace ngn
