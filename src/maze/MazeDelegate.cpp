// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "MazeDelegate.hpp"

#include "audio/Audio.hpp"
#include "gfx/FontMaker.hpp"
#include "gfx/UiRenderer.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "GameStage.hpp"
#include "MazeAssets.hpp"

ngn::ApplicationConfig MazeDelegate::applicationConfig(ngn::Application* app)
{
    NGN_UNUSED(app);

    return {
        .windowWidth = 1024,
        .windowHeight = 768,
        .windowTitle = "Maze ][",
        .windowResizeable = false,

        // 1MB should be way to much for the game but nothing for a modern system
        .requiredMemory = 1 * 1024 * 1024,

        .spriteRenderer = true,
        .spriteBatchCount = 1024,

        .fontRenderer = true,
        .fontBatchCount = 128,

        .audio = true,

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        .debugRenderer = true,
        .debugBatchCount = 65536,
#endif
    };
}

ngn::ApplicationStage* MazeDelegate::onInit(ngn::Application* app)
{
    app_ = app;

    loadAssets(app);

    // loadingStage_ = LoadingStage{app};
    gameStage_ = new GameStage{this};

    // return loadingStage_;
    return gameStage_;
}

void MazeDelegate::onDone(ngn::Application* app)
{
    NGN_UNUSED(app);

    delete gameStage_;
    // delete loadingStage_;

    ngn::log::debug("Max rendered sprites: {}", app->spriteRenderer()->maxRenderedSptrites());
    ngn::log::debug("Max rendered font glyphs: {}", app->uiRenderer()->maxRenderedSptrites());
}

void MazeDelegate::loadAssets(ngn::Application* app)
{
    resources_.spriteTexture = app->spriteRenderer()->addTexture(maze::assets::textures_png());
    const auto& textureAtlas = app->spriteRenderer()->texture(resources_.spriteTexture);

    resources_.uiTexture = app->uiRenderer()->addTexture(textureAtlas.view);

    ngn::FontMaker fontMaker{app->renderer(), 256};
    fontMaker.addFont(maze::assets::liberation_mono_ttf(), 20);
    fontMaker.addFont(maze::assets::liberation_mono_ttf(), 12);
    app->uiRenderer()->setFontCollection(fontMaker.compile());

    resources_.playerShotSoundData = app->audio()->loadOGG(maze::assets::shoot_ogg());
    resources_.enemyShotSoundData = app->audio()->loadOGG(maze::assets::enemy_shoot_ogg());
    resources_.explosionOneSoundData = app->audio()->loadOGG(maze::assets::explosion_one_ogg());
    resources_.explosionTwoSoundData = app->audio()->loadOGG(maze::assets::explosion_two_ogg());
    resources_.laserHitWallSoundData = app->audio()->loadOGG(maze::assets::laser_hit_wall_ogg());
    resources_.boostSoundData = app->audio()->loadOGG(maze::assets::boost_ogg());
}
