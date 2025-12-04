// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Application.hpp"
#include "gfx/SpritePipeline.hpp"
#include <chrono>

namespace ngn {
class Buffer;
class FontRenderer;
class Renderer;
class SpriteRenderer;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
class DebugRenderer;
#endif
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

    ngn::SpriteRenderer* spriteRenderer_;
    ngn::FontRenderer* fontRenderer_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    ngn::DebugRenderer* debugRenderer_;
#endif

    std::vector<ngn::SpriteVertex> sprites_;

    uint64_t frameCount_;
    std::chrono::high_resolution_clock::time_point frameCounterLastCheck_;
};
