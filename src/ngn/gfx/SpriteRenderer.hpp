// Coyright 2025, Daniel Volk <mail@volkarts.com>
// SDX-License-Identifier: <LICENSE>

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include "SpritePipeline.hpp"
#include "Uniforms.hpp"
#include <entt/fwd.hpp>

namespace ngn {

class Buffer;
class CommandBuffer;
class Image;
class ImageView;
class Renderer;
class Sampler;

class SpriteRenderer
{
public:
    SpriteRenderer(Renderer* renderer, uint32_t batchSize);
    ~SpriteRenderer();

    uint32_t addImages(std::span<const BufferView> images);
    uint32_t addImages(std::span<const Image* const> images);

    void updateView(const glm::mat4& view);
    void updateView(const glm::mat4& view, uint32_t frameIndex);
    void renderSprite(const SpriteVertex& vertex);

    void renderSpriteComponents(entt::registry* registry);

    void draw(CommandBuffer* commandBuffer);

private:
    struct UniformBuffer
    {
        Buffer* buffer;
        std::span<ViewProjection> mapped;
    };

    struct Texture
    {
        const Image* image{};
        const ImageView* view{};
        const Sampler* sampler{};
        bool owning{};
    };

    struct Batch
    {
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
    std::array<Batch, MaxFramesInFlight> batches_;

    NGN_DISABLE_COPY_MOVE(SpriteRenderer)
};

} // namesace ngn
