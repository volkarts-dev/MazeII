// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Application.hpp"
#include "Types.hpp"
#include <chrono>

namespace ngn {
class Buffer;
class Renderer;
class Pipeline;
class Sampler;
class SpritePipeline;
} // namespace nng

class TestBedDelegate : public ngn::ApplicationDelegate
{
public:
    TestBedDelegate();
    ~TestBedDelegate() override = default;

    bool onInit(ngn::Application* app) override;
    void onDone(ngn::Application* app) override;
    void onUpdate(ngn::Application* app, float deltaTime) override;
    void onDraw(ngn::Application* app, float deltaTime) override;

private:
    ngn::Renderer* renderer_;

    ngn::SpritePipeline* spritePipeline_;

    std::array<ngn::Buffer*, ngn::MaxFramesInFlight> uniformBuffers_;

    ngn::Image* blackTexture_;
    ngn::ImageView* blackTextureView_;
    ngn::Sampler* blackTextureSampler_;

    ngn::Image* textureAtlas_;
    ngn::ImageView* textureAtlasView_;
    ngn::Sampler* textureAtlasSampler_;

    ngn::Buffer* spriteBuffer_;

    uint64_t frameCount_;
    std::chrono::high_resolution_clock::time_point frameCounterLastCheck_;
};
