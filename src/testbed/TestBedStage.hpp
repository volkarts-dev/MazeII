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

    void onActivate() override;
    void onDeactivate() override;

    void onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods) override;

    void onUpdate(float deltaTime) override;

private:
    ngn::Application* app_;
    std::vector<entt::entity> walls_;
    entt::entity player_;
    entt::entity enemy_;
    entt::entity obstacle_;
};
