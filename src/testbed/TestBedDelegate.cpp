// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "TestBedDelegate.hpp"

#include "gfx/Buffer.hpp"
#include "gfx/CommandBuffer.hpp"
#include "gfx/FontMaker.hpp"
#include "gfx/FontRenderer.hpp"
#include "gfx/Image.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "Logging.hpp"
#include "TestBedAssets.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/gtc/constants.hpp>

TestBedDelegate::TestBedDelegate()
{
}

bool TestBedDelegate::onInit(ngn::Application* app)
{
    renderer_ = app->renderer();

    // ****************************************************

    spriteRenderer_ = new ngn::SpriteRenderer{renderer_, 1024};

    spriteRenderer_->addImages({{testbed::assets::player_png()}});

    // ****************************************************

    ngn::FontMaker fontMaker{renderer_, 256};

    fontMaker.addFont(testbed::assets::liberation_mono_ttf(), 20);

    fontRenderer_ = new ngn::FontRenderer{spriteRenderer_, fontMaker.compile()};

    // ****************************************************

    frameCounterLastCheck_ = ngn::Clock::now();











    // TEMP
    sprites_.resize(4);

    sprites_[0] = {
        .position = {150, 150},
        .rotation = glm::pi<float>() / 7.0f,
        .scale = {100, 100},
        .color = {1.0, 1.0, 1.0, 1.0},
        .texIndex = 1,
        .texCoords = {0.0, 0.0, 64.0, 64.0},
    };

    sprites_[1] = {
        .position = {550, 150},
        .rotation = 0.0f,
        .scale = {100, 100},
        .color = {0.0, 1.0, 0.0, 1.0},
        .texIndex = 0,
        .texCoords = {0.0, 0.0, 0.0, 0.0},
    };

    sprites_[2] = {
        .position = {550, 450},
        .rotation = 0.0f,
        .scale = {100, 100},
        .color = {0.0, 0.0, 1.0, 1.0},
        .texIndex = 0,
        .texCoords = {0.0, 0.0, 0.0, 0.0},
    };

    sprites_[3] = {
        .position = {150, 450},
        .rotation = 0.0f,
        .scale = {100, 100},
        .color = {1.0, 1.0, 0.0, 1.0},
        .texIndex = 0,
        .texCoords = {0.0, 0.0, 0.0, 0.0},
    };

    // /TEMP

    return true;
}

void TestBedDelegate::onDone(ngn::Application* app)
{
    NGN_UNUSED(app);

    delete fontRenderer_;
    delete spriteRenderer_;
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

    spriteRenderer_->updateView(
                glm::lookAt(
                    glm::vec3(0.0f, 0.0f, -10.0f),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                ));

    // ****************************************************

    sprites_[0].rotation += glm::pi<float>() / 10.0f * deltaTime;
    sprites_[1].rotation += -glm::pi<float>() / 50.0f * deltaTime;
    sprites_[2].rotation += glm::pi<float>() / 20.0f * deltaTime;
    sprites_[3].rotation += -glm::pi<float>() / 2.0f * deltaTime;

    for (uint32_t i = 0; i < sprites_.size(); i++)
    {
        spriteRenderer_->renderSprite(sprites_[i]);
    }

    // ****************************************************

    fontRenderer_->drawText(0, "Hello MazeII", 400, 50);
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

    spriteRenderer_->draw(commandBuffer);

    commandBuffer->end();

    renderer_->submit(commandBuffer);

    renderer_->endFrame(imageIndex);
}
