// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "FontMaker.hpp"

#include "Image.hpp"
#include "FontCollection.hpp"
#include "Renderer.hpp"
#include "Types.hpp"
#include "Macros.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <bit>

namespace ngn {

namespace {

class Freetype
{
public:
    Freetype()
    {
        FT_Error error{};

        error = FT_Init_FreeType(&library_);
        if (error)
            throw std::runtime_error("FT error in FT_Init_FreeType()");
    }

    ~Freetype()
    {
        FT_Done_FreeType(library_);
    }

    operator FT_Library() { return library_; }

private:
    FT_Library library_;

    NGN_DISABLE_COPY_MOVE(Freetype)
};

class Face
{
public:
    Face(Freetype& freetype, BufferView buffer, uint32_t size) :
        freetype_{freetype}
    {
        FT_Error error{};

        error = FT_New_Memory_Face(freetype_, buffer.data(), static_cast<FT_Long>(buffer.size()), 0, &face_);
        if (error)
            throw std::runtime_error("FT error in FT_New_Memory_Face()");

        error = FT_Set_Pixel_Sizes(face_, 0, size);
        if (error)
            throw std::runtime_error("FT error in FT_Set_Pixel_Sizes()");
    }

    ~Face()
    {
        FT_Done_Face(face_);
    }

    const FT_Glyph_Metrics& glyphMetrics(uint32_t ch)
    {
        FT_Error error{};

        const auto index = FT_Get_Char_Index(face_, ch);

        error = FT_Load_Glyph(face_, index, FT_LOAD_DEFAULT);
        if (error)
            throw std::runtime_error("FT error in FT_Load_Glyph()");

        return face_->glyph->metrics;
    }

    const FT_GlyphSlot& renderGlyph(uint32_t ch)
    {
        FT_Error error{};

        error = FT_Load_Char(face_, ch,  FT_LOAD_RENDER);
        if (error)
            throw std::runtime_error("FT error in FT_Load_Char()");

        return face_->glyph;
    }

private:
    Freetype& freetype_;
    FT_Face face_;

    NGN_DISABLE_COPY_MOVE(Face)
};

struct CompileState
{
    CompileState(uint32_t _imageDimension) :
        imageDimension{_imageDimension}
    {
    }

    uint32_t imageDimension;

    uint32_t posX{};
    uint32_t posY{};
    uint32_t width{};
    uint32_t height{};
    uint32_t currentMaxRowHeight{};
    uint32_t maxPosX{};

    void update(const FT_Glyph_Metrics& metrics)
    {
        posX += width;
        if (posX > maxPosX)
            maxPosX= posX;

        width = static_cast<uint32_t>(metrics.width / 64);
        height = static_cast<uint32_t>(metrics.height / 64);

        if (height > currentMaxRowHeight)
            currentMaxRowHeight = height;

        if (posX + width > imageDimension)
        {
            posX = 0;
            posY += currentMaxRowHeight;
            if (posY >= imageDimension)
                throw std::runtime_error("Image is to small for the fonts to render;");
            currentMaxRowHeight = height;
        }
    }

    uint32_t calculatedWidth() const
    {
        return maxPosX;
    }

    uint32_t calculatedHeight() const
    {
        return posY + currentMaxRowHeight;
    }
};

void copyGlyph(std::vector<uint8_t>& imageData, const FT_GlyphSlot& glyph, const CompileState& state)
{
    for (uint32_t y = 0; y < glyph->bitmap.rows; y++)
    {
        for (uint32_t x = 0; x < glyph->bitmap.width; x++)
        {
            uint32_t srcPos = y * glyph->bitmap.width + x;
            uint32_t destPos = ((state.posY + y) * state.imageDimension + (state.posX + x)) * 4;

            imageData[destPos + 0] = 255;
            imageData[destPos + 1] = 255;
            imageData[destPos + 2] = 255;
            imageData[destPos + 3] = glyph->bitmap.buffer[srcPos];
        }
    }
}

} // namespace

// *********************************************************************************************************************

FontMaker::FontMaker(Renderer* renderer, uint32_t imageDimension) :
    renderer_(renderer),
    imageDimension_{imageDimension}
{
}

uint32_t FontMaker::addFont(BufferView font, uint32_t size)
{
    fontInfos_.push_back({.font = font, .size = size});
    return static_cast<uint32_t>(fontInfos_.size() - 1);
}

FontCollection* FontMaker::compile()
{
    Freetype freetype;

    CompileState state{imageDimension_};

    for (const auto& fontInfo : fontInfos_)
    {
        Face face(freetype, fontInfo.font, fontInfo.size);

        for (uint32_t ch = 32; ch < 256; ch++)
        {
            const auto& metrics = face.glyphMetrics(ch);

            state.update(metrics);
        }
    }

    //auto imageDimensionF = static_cast<float>(imageDimension_);

    std::vector<std::vector<GlyphInfo>> glyphInfos{};
    glyphInfos.reserve(fontInfos_.size());
    std::vector<uint8_t> imageData{};
    imageData.resize(imageDimension_ * imageDimension_ * 4);

    state = CompileState{imageDimension_};

    for (const auto& fontInfo : fontInfos_)
    {
        auto& fontGlypInfo = glyphInfos.emplace_back();
        fontGlypInfo.reserve(256 - 32);

        Face face(freetype, fontInfo.font, fontInfo.size);

        for (uint32_t ch = 32; ch < 256; ch++)
        {
            const auto& glyph = face.renderGlyph(ch);
            const auto& metrics = glyph->metrics;

            state.update(metrics);

            fontGlypInfo.push_back({
                .bearing = {metrics.horiBearingX / 64, -metrics.horiBearingY / 64},
                .size = {state.width, state.height},
                .texCoords = {
                    /*
                    (static_cast<float>(state.posX) + 0.0f) / imageDimensionF,
                    (static_cast<float>(state.posY) + 0.0f) / imageDimensionF,
                    (static_cast<float>(state.posX + state.width) + 0.0f) / imageDimensionF,
                    (static_cast<float>(state.posY + state.height) + 0.0f) / imageDimensionF,
                    */
                   (static_cast<float>(state.posX) + 0.0f),
                   (static_cast<float>(state.posY) + 0.0f),
                   (static_cast<float>(state.posX + state.width) + 0.0f),
                   (static_cast<float>(state.posY + state.height) + 0.0f),
                },
                .advance = static_cast<float>(metrics.horiAdvance / 64),
            });

            copyGlyph(imageData, glyph, state);
        }
    }

    auto loader = ImageLoader::createFromBitmap(renderer_, imageDimension_, imageDimension_, imageData);
    auto* image = new Image{loader};

    return new FontCollection{std::move(glyphInfos), image};
}

} // namespace ngn
