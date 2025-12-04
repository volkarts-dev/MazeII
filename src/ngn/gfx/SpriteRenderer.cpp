// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "SpriteRenderer.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Image.hpp"
#include "gfx/Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace ngn {

SpriteRenderer::SpriteRenderer(Renderer* renderer, uint32_t batchSize) :
    renderer_{renderer},
    spritePipeline_{new SpritePipeline{renderer_}}
{
    BufferConfig uniformBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eUniformBuffer,
        sizeof(ViewProjection)
    };
    uniformBufferConfig.hostVisible = true;

    // TODO Use one buffer for all uniforms
    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        uniformBuffers_[i].buffer = new Buffer{uniformBufferConfig};
        uniformBuffers_[i].mapped = uniformBuffers_[i].buffer->map<ViewProjection>();

        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers_[i].buffer->handle(),
            .offset = 0,
            .range = sizeof(ViewProjection),
        };
        spritePipeline_->updateDescriptorSet(bufferInfo, i, 0);
    }

    // ****************************************************

    textures_.resize(1);

    std::array<uint8_t, 4> whiteTextureData{255, 255, 255, 255};
    const auto whiteTextureLoader = ImageLoader::createFromBitmap(renderer_, 1, 1, whiteTextureData);

    textures_[0].image = new Image{whiteTextureLoader};
    textures_[0].view = new ImageView{textures_[0].image};
    textures_[0].sampler = new Sampler{renderer_, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge};
    textures_[0].owning = true;

    vk::DescriptorImageInfo imageInfo{
        .sampler = textures_[0].sampler->handle(),
        .imageView = textures_[0].view->handle(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    for (uint32_t f = 0; f < MaxFramesInFlight; f++)
    {
        for (uint32_t i = 0; i < MaxSpritePipelineTextures; i++)
        {
            spritePipeline_->updateDescriptorSet(imageInfo, f, 1, i);
        }
    }

    // ****************************************************

    ngn::BufferConfig spriteBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eVertexBuffer,
        sizeof(ngn::SpriteVertex) * batchSize
    };
    spriteBufferConfig.hostVisible = true;
    for (uint32_t f = 0; f < MaxFramesInFlight; f++)
    {
        batches_[f].buffer = new ngn::Buffer{spriteBufferConfig};
        batches_[f].mapped = batches_[f].buffer->map<SpriteVertex>();
        batches_[f].count = 0;
    }
}

SpriteRenderer::~SpriteRenderer()
{
    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        batches_[f].buffer->unmap();
        delete batches_[f].buffer;
    }

    for (uint32_t i = 0; i < textures_.size(); i++)
    {
        delete textures_[i].sampler;
        delete textures_[i].view;
        if (textures_[i].owning)
            delete textures_[i].image;
    }

    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        uniformBuffers_[f].buffer->unmap();
        delete uniformBuffers_[f].buffer;
    }

    delete spritePipeline_;
}

uint32_t SpriteRenderer::addImages(std::span<const BufferView> images)
{
    const uint32_t startIndex = static_cast<uint32_t>(textures_.size());
    const uint32_t endIndex = startIndex + static_cast<uint32_t>(images.size());

    assert(startIndex + images.size() <= MaxSpritePipelineTextures);

    textures_.resize(startIndex + images.size());

    for (uint32_t i = startIndex; i < endIndex; i++)
    {
        const auto textureAtlasLoader = ImageLoader::loadFromBuffer(renderer_, images[i - startIndex]);

        addImage(i, new Image{textureAtlasLoader}, true);
    }

    return startIndex;
}

uint32_t SpriteRenderer::addImages(std::span<const Image* const> images)
{
    const uint32_t startIndex = static_cast<uint32_t>(textures_.size());
    const uint32_t endIndex = startIndex + static_cast<uint32_t>(images.size());

    assert(startIndex + images.size() <= MaxSpritePipelineTextures);

    textures_.resize(startIndex + images.size());

    for (uint32_t i = startIndex; i < endIndex; i++)
    {
        addImage(i, images[i - startIndex], false);
    }

    return startIndex;
}

void SpriteRenderer::updateView(const glm::mat4& view)
{
    const auto screenSize = renderer_->swapChainExtent();

    auto& ubo = uniformBuffers_[renderer_->currentFrame()];

    ubo.mapped[0].view = view;
    ubo.mapped[0].proj = glm::ortho(
                0.0f, static_cast<float>(screenSize.width),
                0.0f, static_cast<float>(screenSize.height)
                );
}

void SpriteRenderer::addImage(uint32_t index, const Image* image, bool owning)
{
    textures_[index].image = image;
    textures_[index].view = new ImageView{textures_[index].image};
    textures_[index].sampler = new Sampler{renderer_, vk::Filter::eNearest, vk::SamplerAddressMode::eClampToEdge, true};
    textures_[index].owning = owning;

    vk::DescriptorImageInfo imageInfo{
        .sampler = textures_[index].sampler->handle(),
        .imageView = textures_[index].view->handle(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    for (uint32_t f = 0; f < MaxFramesInFlight; f++)
    {
        spritePipeline_->updateDescriptorSet(imageInfo, f, 1, index);
    }
}

void SpriteRenderer::renderSprite(const SpriteVertex& vertex)
{
    static_assert(std::is_trivially_copyable_v<SpriteVertex>, "SpriteVertex is not trivially copyable");

    auto& batch = batches_[renderer_->currentFrame()];

    assert(batch.count < batch.buffer->size() / sizeof(SpriteVertex));

    std::memcpy(&batch.mapped[batch.count], &vertex, sizeof(SpriteVertex));

    batch.count++;
}

void SpriteRenderer::draw(CommandBuffer* commandBuffer)
{
    const auto frameIndex = renderer_->currentFrame();
    commandBuffer->bindPipeline(spritePipeline_->pipeline());
    commandBuffer->bindDescriptorSet(spritePipeline_->pipeline(), spritePipeline_->descriptorSet(frameIndex));

    auto& batch = batches_[frameIndex];

    commandBuffer->bindVertexBuffer(batch.buffer);
    commandBuffer->draw(batch.count);

    batch.count = 0;
}

} // namespace ngn
