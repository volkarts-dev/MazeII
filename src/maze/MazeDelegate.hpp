// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Application.hpp"
#include "gfx/GfxIds.hpp"

namespace ngn {
class AudioBuffer;
} // namespace ngn

class GameStage;

class Resources
{
public:
    ngn::ImageId textureAtlas;
    ngn::AudioBuffer* playerShotSoundData;
    ngn::AudioBuffer* enemyShotSoundData;
    ngn::AudioBuffer* explosionSoundData;
    ngn::AudioBuffer* laserHitWallSoundData;
};

class MazeDelegate : public ngn::ApplicationDelegate
{
public:
    MazeDelegate() = default;
    ~MazeDelegate() override = default;

    ngn::ApplicationConfig applicationConfig(ngn::Application* app) override;

    ngn::ApplicationStage* onInit(ngn::Application* app) override;
    void onDone(ngn::Application* app) override;

    ngn::Application* app() const { return app_; }
    const Resources& resources() const { return resources_; }

private:
    void loadAssets(ngn::Application* app);

private:
    ngn::Application* app_;
    Resources resources_;
    // LoadingStage* loadingStage_;
    GameStage* gameStage_;
};
