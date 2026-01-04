// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "TestBedDelegate.hpp"

#include "TestBedStage.hpp"

TestBedDelegate::TestBedDelegate()
{
}

std::size_t TestBedDelegate::requiredFrameMemeory()
{
    return 100 * 1024 * 1024;
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
