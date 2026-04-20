// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "SpriteRenderer.hpp"

#include "Buffer.hpp"
#include "CommonComponents.hpp"
#include "Instrumentation.hpp"
#include "CommandBuffer.hpp"
#include "gfx/GfxComponents.hpp"
#include "gfx/FontCollection.hpp"
#include "gfx/Renderer.hpp"
#include "phys/PhysComponents.hpp"
#include <entt/entt.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ngn {

SpriteRenderer::SpriteRenderer(Renderer* renderer, uint32_t batchSize) :
    renderer_{renderer},
    spritePipeline_{new SpritePipeline{renderer_}},
    fontCollection_{},
    fontImageId_{},
    batches_{},
    staticCount_{},
    maxRenderedSprites_{}
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
    textures_[0].sampler = new Sampler{renderer_, {vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge}};
    textures_[0].ownsImage = true;
    textures_[0].ownsView = true;

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
    delete fontCollection_;

    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        batches_[f].buffer->unmap();
        delete batches_[f].buffer;
    }

    for (uint32_t i = 0; i < textures_.size(); i++)
    {
        delete textures_[i].sampler;

        if (textures_[i].view && textures_[i].ownsView)
            delete textures_[i].view;

        if (textures_[i].image && textures_[i].ownsImage)
            delete textures_[i].image;
    }

    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        uniformBuffers_[f].buffer->unmap();
        delete uniformBuffers_[f].buffer;
    }

    delete spritePipeline_;
}

TextureId SpriteRenderer::addTexture(const BufferView& image, const SamplerCreateInfo& samplerCreateInfo)
{
    const auto index = static_cast<uint32_t>(textures_.size());
    assert(index <= MaxSpritePipelineTextures);
    const auto textureId = static_cast<TextureId>(index);

    const auto textureAtlasLoader = ImageLoader::loadFromBuffer(renderer_, image);
    const Image* img = new Image{textureAtlasLoader};

    textures_.push_back({
        .image = img,
        .view = new ImageView{img},
        .sampler = new Sampler{renderer_, samplerCreateInfo},
        .ownsImage = true,
        .ownsView = true,
    });

    updateSamplerDescriptors(textureId);

    return textureId;
}

TextureId SpriteRenderer::addTexture(const Image* image, const SamplerCreateInfo& samplerCreateInfo)
{
    const auto index = static_cast<uint32_t>(textures_.size());
    assert(index <= MaxSpritePipelineTextures);
    const auto textureId = static_cast<TextureId>(index);

    textures_.push_back({
        .image = image,
        .view = new ImageView{image},
        .sampler = new Sampler{renderer_, samplerCreateInfo},
        .ownsImage = false,
        .ownsView = true,
    });

    updateSamplerDescriptors(textureId);

    return textureId;
}

TextureId SpriteRenderer::addTexture(const ImageView* image, const SamplerCreateInfo& samplerCreateInfo)
{
    const auto index = static_cast<uint32_t>(textures_.size());
    assert(index <= MaxSpritePipelineTextures);
    const auto textureId = static_cast<TextureId>(index);

    textures_.push_back({
        .image = nullptr,
        .view = image,
        .sampler = new Sampler{renderer_, samplerCreateInfo},
        .ownsImage = false,
        .ownsView = false,
    });

    updateSamplerDescriptors(textureId);

    return textureId;
}

TextureId SpriteRenderer::reserveTexture(const SamplerCreateInfo& samplerCreateInfo)
{
    const auto index = static_cast<uint32_t>(textures_.size());
    assert(index <= MaxSpritePipelineTextures);
    const auto textureId = static_cast<TextureId>(index);

    textures_.push_back({
        .image = nullptr,
        .view = nullptr,
        .sampler = new Sampler{renderer_, samplerCreateInfo},
        .ownsImage = false,
        .ownsView = false,
    });

    return textureId;
}

TextureId SpriteRenderer::setFontCollection(FontCollection* fontCollection)
{
    fontCollection_ = fontCollection;
    fontImageId_ = addTexture(fontCollection_->image());
    return fontImageId_;
}

const SpriteRenderer::Texture& SpriteRenderer::texture(TextureId textureId) const
{
    return textures_[std::to_underlying(textureId)];
}

void SpriteRenderer::updateView(const glm::mat4& view)
{
    updateView(view, renderer_->currentFrame());
}

void SpriteRenderer::updateView(const glm::mat4& view, uint32_t frameIndex)
{
    auto& ubo = uniformBuffers_[frameIndex];
    ubo.mapped[0].view = view;
}

void SpriteRenderer::updateProj(const glm::mat4& proj)
{
    updateProj(proj, renderer_->currentFrame());
}

void SpriteRenderer::updateProj(const glm::mat4& proj, uint32_t frameIndex)
{
    auto& ubo = uniformBuffers_[frameIndex];
    ubo.mapped[0].proj = proj;
}

void SpriteRenderer::updateSamplerDescriptor(TextureId textureId, uint32_t frameIndex, const ImageView* imageView)
{
    const auto index = std::to_underlying(textureId);

    vk::DescriptorImageInfo imageInfo{
        .sampler = textures_[index].sampler->handle(),
        .imageView = imageView->handle(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };

    spritePipeline_->updateDescriptorSet(imageInfo, frameIndex, 1, index);
}

void SpriteRenderer::updateSamplerDescriptors(TextureId textureId)
{
    const auto index = std::to_underlying(textureId);

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

void SpriteRenderer::renderText(FontId font, std::string_view text, glm::vec2 pos)
{
    for (uint32_t i = 0; i < text.size(); i++)
    {
        const auto& glyph = fontCollection_->glyphInfo(font)[static_cast<uint8_t>(text[i]) - 32];

        renderSprite(SpriteVertex{
            .position = pos + glyph.size / 2.f + glyph.bearing,
            .rotation = 0.0f,
            .scale = glyph.size,
            .color = {1.0, 1.0, 1.0, 1.0},
            .texCoords = glyph.texCoords,
            .texIndex = fontImageId_,
        });

        pos.x += glyph.advance;
    }
}

#if 0
// Splitting static and dynamic sprites leads to reduced upload time to the GPU
// but the draw call tooks longer. As this need more invastivation, disbale it for now

void SpriteRenderer::prepareStaticSpriteComponents(entt::registry* registry)
{
    for (uint32_t f = 0; f < MaxFramesInFlight; f++)
    {
        batches_[f].count = 0;
        renderSpriteComponentsImpl<ngn::StaticTag>(registry, f);
    }
    staticCount_ = batches_[0].count;
}

void SpriteRenderer::renderSpriteComponents(entt::registry* registry)
{
    renderSpriteComponentsImpl<ngn::DynamicTag>(registry, renderer_->currentFrame());
}

#else
// This enables the old behavior: Skip upload static sprite at startup and upload all sprites at every frame

void SpriteRenderer::prepareStaticSpriteComponents(entt::registry* registry)
{
    NGN_UNUSED(registry);
}

void SpriteRenderer::renderSpriteComponents(entt::registry* registry)
{
    renderSpriteComponentsImpl(registry, renderer_->currentFrame());
}

#endif

template<typename... TagsT>
void SpriteRenderer::renderSpriteComponentsImpl(entt::registry* registry, uint32_t frame)
{
    NGN_INSTRUMENT_FUNCTION();

    auto& batch = batches_[frame];

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

void SpriteRenderer::draw(CommandBuffer* commandBuffer)
{
    const auto frameIndex = renderer_->currentFrame();
    commandBuffer->bindPipeline(spritePipeline_->pipeline());
    commandBuffer->bindDescriptorSet(spritePipeline_->pipeline(), spritePipeline_->descriptorSet(frameIndex));

    auto& batch = batches_[frameIndex];
    commandBuffer->bindVertexBuffer(batch.buffer);
    commandBuffer->draw(batch.count);

    if (batch.count > maxRenderedSprites_)
        maxRenderedSprites_ = batch.count;

    batch.count = staticCount_;
}

} // namespace ngn

NGN_INSTRUMENTATION_EPILOG(SpriteRenderer)
