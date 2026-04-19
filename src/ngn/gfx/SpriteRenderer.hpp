// Coyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Buffer.hpp"
#include "CommonComponents.hpp"
#include "Instrumentation.hpp"
#include "Macros.hpp"
#include "Types.hpp"
#include "gfx/GfxComponents.hpp"
#include "gfx/GfxIds.hpp"
#include "gfx/Image.hpp"
#include "gfx/Renderer.hpp"
#include "gfx/SpritePipeline.hpp"
#include "gfx/Uniforms.hpp"
#include <entt/entt.hpp>

namespace ngn {

class CommandBuffer;
class FontCollection;
class Image;
class ImageView;
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

    template<typename... TagsT>
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

template<typename... TagsT>
void SpriteRenderer::renderSpriteComponents(entt::registry* registry)
{
    NGN_INSTRUMENT_FUNCTION();

    auto& batch = batches_[renderer_->currentFrame()];

    auto sprites = registry->view<const Position, const Sprite, ActiveTag, TagsT...>();
    for (auto [e, pos, spr] : sprites.each())
    {
        assert(batch.count < batch.buffer->size() / sizeof(SpriteVertex));

        NGN_INSTRUMENT_BLOCK_BANDWIDTH_VAR(ls, "<load-sprite>", sizeof(Sprite));

        auto [rot, sca] = registry->try_get<const Rotation, const Scale>(e);

        NGN_SCOPETIMER_STOP(ls)

        NGN_INSTRUMENT_BLOCK_BANDWIDTH_VAR(ps, "<push-sprite>", sizeof(SpriteVertex));

        auto& v = batch.mapped[batch.count];
        v.position = pos.value;
        v.rotation = rot ? rot->angle : 0.0f;
        v.scale = spr.size * (sca ? sca->value : glm::vec2{1, 1});
        v.color = spr.color;
        v.texCoords = spr.texCoords;
        v.texIndex = spr.texture;

        batch.count++;

        NGN_SCOPETIMER_STOP(ps)
    }
}

} // namesace ngn
