// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "FontCollection.hpp"

#include "Image.hpp"

namespace ngn {

FontCollection::FontCollection(std::vector<std::vector<GlyphInfo>>&& glyphInfo, Image* image) :
    glyphInfo_{std::move(glyphInfo)},
    image_{image}
{
}

FontCollection::~FontCollection()
{
    delete image_;
}

glm::vec2 FontCollection::textDimension(FontId font, std::string_view text) const
{
    float width = 0;
    float minHeight = 0;
    float maxHeight = 0;

    for (uint32_t i = 0; i < text.size(); i++)
    {
        const auto& glyph = glyphInfo(font)[static_cast<uint8_t>(text[i]) - 32];

        width += glyph.advance;

        minHeight = glm::min(minHeight, glyph.bearing.y);
        maxHeight = glm::max(maxHeight, glyph.bearing.y + glyph.size.y);
    }

    return {width, maxHeight - minHeight};
}

} // namespace ngn
