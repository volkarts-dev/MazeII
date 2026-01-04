// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Application.hpp"

namespace ngn {
class Buffer;
class FontRenderer;
class Renderer;
class SpriteRenderer;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
class DebugRenderer;
#endif
} // namespace nng

class TestBedStage;

class TestBedDelegate : public ngn::ApplicationDelegate
{
public:
    TestBedDelegate();
    ~TestBedDelegate() override = default;

    std::size_t requiredFrameMemeory() override;

    ngn::ApplicationStage* onInit(ngn::Application* app) override;
    void onDone(ngn::Application* app) override;

private:
    TestBedStage* stage_;
};
