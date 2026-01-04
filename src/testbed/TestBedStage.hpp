// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Application.hpp"

namespace ngn {
class Buffer;
class Renderer;
} // namespace nng

class TestBedStage : public ngn::ApplicationStage
{
public:
    TestBedStage(ngn::Application* app);
    ~TestBedStage() override;

    void onActivate(ngn::Application* app) override;
    void onDeactivate(ngn::Application* app) override;

    void onKeyEvent(ngn::Application* app, int action, int key) override;

    void onUpdate(ngn::Application* app, float deltaTime) override;
    void onDraw(ngn::Application* app, float deltaTime) override;

private:
    std::vector<entt::entity> walls_;
    entt::entity player_;
    entt::entity enemy_;
    entt::entity obstacle_;
};
