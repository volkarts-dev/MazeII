// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include <glm/glm.hpp>

namespace ngn {

class Image;

class GlyphInfo
{
public:
    const glm::vec2 bearing;
    const glm::vec2 size;
    const glm::vec4 texCoords;
    const float advance;
};

class FontCollection
{
public:
    FontCollection(std::vector<std::vector<GlyphInfo>>&& glyphInfo, Image* image);
    ~FontCollection();

    const std::vector<GlyphInfo>& glyphInfo(uint32_t fontIndex) const { return glyphInfo_[fontIndex]; }
    const Image* image() const { return image_; }

private:
    std::vector<std::vector<GlyphInfo>> glyphInfo_;
    Image* image_;

    NGN_DISABLE_COPY_MOVE(FontCollection)
};

} // namespace ngn
