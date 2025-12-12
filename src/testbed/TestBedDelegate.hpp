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

    std::size_t requiredFrameMemeory() override;

    bool onInit(ngn::Application* app) override;
    void onDone(ngn::Application* app) override;

    void onKeyEvent(ngn::Application* app, int action, int key) override;

    void onUpdate(ngn::Application* app, float deltaTime) override;
    void onDraw(ngn::Application* app, float deltaTime) override;

private:
    ngn::Renderer* renderer_;
    entt::registry* registry_;

    // TODO move these renderers  to application
    ngn::SpriteRenderer* spriteRenderer_;
    ngn::FontRenderer* fontRenderer_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    ngn::DebugRenderer* debugRenderer_;
#endif

    entt::entity player_;
    entt::entity enemy_;

    std::vector<ngn::SpriteVertex> sprites_;

    uint64_t frameCount_;
    std::chrono::high_resolution_clock::time_point frameCounterLastCheck_;
};
