// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "Types.hpp"

namespace ngn {

class FontCollection;
class Image;
class Renderer;

class FontMaker
{
public:
    FontMaker(Renderer* renderer, uint32_t imageDimension);

    uint32_t addFont(BufferView font, uint32_t size);

    FontCollection* compile();

private:
    struct FontInfo
    {
        BufferView font;
        uint32_t size;
    };

    Renderer* renderer_;
    uint32_t imageDimension_;
    std::vector<FontInfo> fontInfos_;
};

} // namespace ngn
