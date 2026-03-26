// Coyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include "gfx/GfxIds.hpp"
#include "gfx/Image.hpp"
#include "gfx/SpritePipeline.hpp"
#include "gfx/Uniforms.hpp"
#include <entt/fwd.hpp>

namespace ngn {

class Buffer;
class CommandBuffer;
class FontCollection;
class Image;
class ImageView;
class Renderer;
class Sampler;

class SpriteRenderer
{
public:
    class Texture
    {
    public:
        const Image* image{};
        const ImageView* view{};
        const Sampler* sampler{};
        bool ownsImage{};
        bool ownsView{};
    };

public:
    SpriteRenderer(Renderer* renderer, uint32_t batchSize);
    virtual ~SpriteRenderer();

    SpritePipeline* pipeline() const { return spritePipeline_; }
    const FontCollection* fontCollection() const { return fontCollection_; }
    uint32_t maxRenderedSptrites() const { return maxRenderedSptrites_; }

    TextureId addTexture(const BufferView& image, const SamplerCreateInfo& samplerCreateInfo = {});
    TextureId addTexture(const Image* image, const SamplerCreateInfo& samplerCreateInfo = {});
    TextureId addTexture(const ImageView* image, const SamplerCreateInfo& samplerCreateInfo = {});
    TextureId setFontCollection(FontCollection* fontCollection);
    TextureId reserveTexture(const SamplerCreateInfo& samplerCreateInfo = {});
    const Texture& texture(TextureId textureId) const;

    void updateSamplerDescriptor(TextureId textureId, uint32_t frameIndex, const ImageView* imageView);

    void updateView(const glm::mat4& view);
    void updateView(const glm::mat4& view, uint32_t frameIndex);
    void updateProj(const glm::mat4& proj);
    void updateProj(const glm::mat4& proj, uint32_t frameIndex);

    void renderSprite(const SpriteVertex& vertex);
    void renderText(FontId font, std::string_view text, glm::vec2 pos);

    void renderSpriteComponents(entt::registry* registry);

    void draw(CommandBuffer* commandBuffer);

private:
    class UniformBuffer
    {
    public:
        Buffer* buffer;
        std::span<ViewProjection> mapped;
    };

    class Batch
    {
    public:
        Buffer* buffer;
        std::span<SpriteVertex> mapped;
        uint32_t count;
    };

private:
    void updateSamplerDescriptors(TextureId textureId);

private:
    Renderer* renderer_;
    SpritePipeline* spritePipeline_;
    std::array<UniformBuffer, MaxFramesInFlight> uniformBuffers_;
    std::vector<Texture> textures_;
    FontCollection* fontCollection_;
    TextureId fontImageId_;
    std::array<Batch, MaxFramesInFlight> batches_;
    uint32_t maxRenderedSptrites_;

    NGN_DISABLE_COPY_MOVE(SpriteRenderer)
};

} // namesace ngn
