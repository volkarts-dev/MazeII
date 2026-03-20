// Coyright 2025, Daniel Volk <mail@volkarts.com>
// SDX-License-Identifier: <LICENSE>

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include "SpritePipeline.hpp"
#include "Uniforms.hpp"
#include "gfx/GfxIds.hpp"
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
    SpriteRenderer(Renderer* renderer, uint32_t batchSize);
    virtual ~SpriteRenderer();

    SpritePipeline* pipeline() const { return spritePipeline_; }

    ImageId reserveTextureSlot();
    ImageId addImages(std::span<const BufferView> images);
    ImageId addImages(std::span<const Image* const> images);
    ImageId setFontCollection(FontCollection* fontCollection);

    void updateView(const glm::mat4& view);
    void updateView(const glm::mat4& view, uint32_t frameIndex);
    void updateProj(const glm::mat4& proj);
    void updateProj(const glm::mat4& proj, uint32_t frameIndex);

    void renderSprite(const SpriteVertex& vertex);
    void renderText(FontId font, std::string_view text, uint32_t x, uint32_t y);

    void renderSpriteComponents(entt::registry* registry);

    void draw(CommandBuffer* commandBuffer);

private:
    class UniformBuffer
    {
    public:
        Buffer* buffer;
        std::span<ViewProjection> mapped;
    };

    class Texture
    {
    public:
        const Image* image{};
        const ImageView* view{};
        const Sampler* sampler{};
        bool owning{};
    };

    class Batch
    {
    public:
        Buffer* buffer;
        std::span<SpriteVertex> mapped;
        uint32_t count;
    };

private:
    void addImage(uint32_t index, const Image* image, bool owning);

private:
    Renderer* renderer_;
    SpritePipeline* spritePipeline_;
    std::array<UniformBuffer, MaxFramesInFlight> uniformBuffers_;
    std::vector<Texture> textures_;
    FontCollection* fontCollection_;
    ImageId fontImageId_;
    std::array<Batch, MaxFramesInFlight> batches_;

    NGN_DISABLE_COPY_MOVE(SpriteRenderer)
};

} // namesace ngn
