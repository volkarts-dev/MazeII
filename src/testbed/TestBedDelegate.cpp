// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "TestBedDelegate.hpp"

#include "gfx/Buffer.hpp"
#include "gfx/CommandBuffer.hpp"
#include "gfx/Image.hpp"
#include "gfx/SpritePipeline.hpp"
#include "Logging.hpp"
#include "TestBedAssets.hpp"
#include <glm/gtc/matrix_transform.hpp>

TestBedDelegate::TestBedDelegate()
{
}

bool TestBedDelegate::onInit(ngn::Application* app)
{
    renderer_ = app->renderer();

    spritePipeline_ = new ngn::SpritePipeline{renderer_};

    // ****************************************************

    ngn::BufferConfig uniformBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eUniformBuffer,
        sizeof(ngn::SpriteUniform)
    };
    uniformBufferConfig.hostVisible = true;

    // TODO Use one buffer for all uniforms
    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        uniformBuffers_[i] = new ngn::Buffer{uniformBufferConfig};

        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers_[i]->handle(),
            .offset = 0,
            .range = sizeof(ngn::SpriteUniform),
        };
        spritePipeline_->updateDescriptorSet(bufferInfo, i, 0);
    }

    // ****************************************************

    std::array<uint8_t, 4> blackTextureData{0, 0, 0, 1};
    const auto blackTextureLoader = ngn::ImageLoader::createFromBitmap(renderer_, 1, 1, blackTextureData);
    blackTexture_ = new ngn::Image{blackTextureLoader};

    blackTextureView_ = new ngn::ImageView{blackTexture_};
    blackTextureSampler_ = new ngn::Sampler{renderer_, vk::Filter::eNearest, vk::SamplerAddressMode::eClampToEdge};

    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        vk::DescriptorImageInfo imageInfo{
            .sampler = blackTextureSampler_->handle(),
            .imageView = blackTextureView_->handle(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };
        for (uint32_t e = 0; e < 10; e++)
        {
            spritePipeline_->updateDescriptorSet(imageInfo, i, 1, e);
        }
    }

    // **********************

    const auto textureAtlasLoader = ngn::ImageLoader::loadFromBuffer(renderer_, testbed::assets::player_png());
    textureAtlas_ = new ngn::Image{textureAtlasLoader};

    textureAtlasView_ = new ngn::ImageView{textureAtlas_};
    textureAtlasSampler_ = new ngn::Sampler{renderer_, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge};

    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        vk::DescriptorImageInfo imageInfo{
            .sampler = textureAtlasSampler_->handle(),
            .imageView = textureAtlasView_->handle(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };
        spritePipeline_->updateDescriptorSet(imageInfo, i, 1, 1);
    }

    // ****************************************************

    ngn::BufferConfig spriteBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eVertexBuffer,
        sizeof(ngn::SpriteVertex) * 100
    };
    spriteBufferConfig.hostVisible = true;
    spriteBuffer_ = new ngn::Buffer{spriteBufferConfig};

    // ****************************************************

    frameCounterLastCheck_ = ngn::Clock::now();











    // TEMP use a sprite batch
    auto verticies = spriteBuffer_->map<ngn::SpriteVertex>();

    verticies[0] = {
        .position = {150, 150},
        .rotation = glm::pi<float>() / 7.0f,
        .scale = {100, 100},
        .color = {1.0, 0.0, 0.0, 1.0},
        .texIndex = 0,
        .texCoords = {0.0, 0.0, 1.0, 1.0},
    };

    verticies[1] = {
        .position = {550, 150},
        .rotation = 0.0f,
        .scale = {100, 100},
        .color = {0.0, 1.0, 0.0, 1.0},
        .texIndex = 99,
        .texCoords = {0.0, 0.0, 0.0, 0.0},
    };

    verticies[2] = {
        .position = {550, 450},
        .rotation = 0.0f,
        .scale = {100, 100},
        .color = {0.0, 0.0, 1.0, 1.0},
        .texIndex = 99,
        .texCoords = {0.0, 0.0, 0.0, 0.0},
    };

    verticies[3] = {
        .position = {150, 450},
        .rotation = 0.0f,
        .scale = {100, 100},
        .color = {1.0, 1.0, 0.0, 1.0},
        .texIndex = 99,
        .texCoords = {0.0, 0.0, 0.0, 0.0},
    };

    spriteBuffer_->unmap();
    // /TEMP

    return true;
}

void TestBedDelegate::onDone(ngn::Application* app)
{
    NGN_UNUSED(app);

    delete spriteBuffer_;

    delete textureAtlasSampler_;
    delete textureAtlasView_;
    delete textureAtlas_;

    delete blackTextureSampler_;
    delete blackTextureView_;
    delete blackTexture_;

    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        delete uniformBuffers_[i];
    }

    delete spritePipeline_;
}

void TestBedDelegate::onUpdate(ngn::Application* app, float deltaTime)
{
    NGN_UNUSED(app);

    const auto currentTime = ngn::Clock::now();
    if (currentTime > frameCounterLastCheck_ + std::chrono::milliseconds(5000))
    {
        const ngn::Duration<double> elapsed = currentTime - frameCounterLastCheck_;
        const auto fps = static_cast<double>(frameCount_) / elapsed.count();
        frameCounterLastCheck_ = currentTime;
        frameCount_ = 0;

        ngn::log::info("FPS: {}", fps);
    }

    frameCount_++;

    // ****************************************************

    const auto screenSize = renderer_->swapChainExtent();

    auto* ubo = uniformBuffers_[renderer_->currentFrame()];
    auto uboRange = ubo->map<ngn::SpriteUniform>();
    uboRange[0].viewProj.view = glm::lookAt(
                glm::vec3(0.0f, 0.0f, -10.0f),
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
                );
    uboRange[0].viewProj.proj = glm::ortho(
                0.0f, static_cast<float>(screenSize.width),
                0.0f, static_cast<float>(screenSize.height)
                );
    ubo->unmap();

    auto verticiesRange = spriteBuffer_->map<ngn::SpriteVertex>();

    verticiesRange[0].rotation += glm::pi<float>() / 10.0f * deltaTime;
    verticiesRange[1].rotation += -glm::pi<float>() / 50.0f * deltaTime;
    verticiesRange[2].rotation += glm::pi<float>() / 20.0f * deltaTime;
    verticiesRange[3].rotation += -glm::pi<float>() / 2.0f * deltaTime;

    spriteBuffer_->unmap();
}

void TestBedDelegate::onDraw(ngn::Application* app, float deltaTime)
{
    NGN_UNUSED(app);
    NGN_UNUSED(deltaTime);

    const auto imageIndex = renderer_->startFrame();
    if (imageIndex == ngn::InvalidIndex)
        return;

    auto* commandBuffer = renderer_->currentCommandBuffer();

    commandBuffer->begin(imageIndex);

    commandBuffer->bindPipeline(spritePipeline_->pipeline());
    commandBuffer->bindDescriptorSet(
                spritePipeline_->pipeline(),
                spritePipeline_->descriptorSet(renderer_->currentFrame()));

    commandBuffer->bindVertexBuffer(spriteBuffer_);

    commandBuffer->draw(4);

    commandBuffer->end();

    renderer_->submit(commandBuffer);

    renderer_->endFrame(imageIndex);
}
