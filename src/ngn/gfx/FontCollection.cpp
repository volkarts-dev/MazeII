// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

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

} // namespace ngn
