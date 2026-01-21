// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "TestBedDelegate.hpp"

#include "TestBedStage.hpp"

TestBedDelegate::TestBedDelegate()
{
}

ngn::ApplicationConfig TestBedDelegate::applicationConfig(ngn::Application* app)
{
    NGN_UNUSED(app);

    return {
        .windowWidth = 800,
        .windowHeight = 600,
        .windowTitle = "Maze][ - TestBed",

        .requiredMemory = 100 * 1024 * 1024,

        .spriteRenderer = true,
        .spriteBatchCount = 1024,
        .fontRenderer = true,

        .audio = false,

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        .debugRenderer = true,
        .debugBatchCount = 1204
#endif
    };
}

ngn::ApplicationStage* TestBedDelegate::onInit(ngn::Application* app)
{
    stage_ = new TestBedStage{app};

    return stage_;
}

void TestBedDelegate::onDone(ngn::Application* app)
{
    NGN_UNUSED(app);

    delete stage_;
}
