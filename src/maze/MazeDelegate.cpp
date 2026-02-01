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

        .requiredMemory = 100 * 1024 * 1024, // TODO st correct memory amount

        .spriteRenderer = true,
        .spriteBatchCount = 16384, // TODO set correct max sprite count

        .fontRenderer = true,
        .fontBatchCount = 16384,

        .audio = true,

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        .debugRenderer = true,
        .debugBatchCount = 16384
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
}

void MazeDelegate::loadAssets(ngn::Application* app)
{
    // TODO move asset loading and destroying off unshared resources into stages when there are more then one

    resources_.textureAtlas = app->spriteRenderer()->addImages({{maze::assets::textures_png()}});

    ngn::FontMaker fontMaker{app->renderer(), 256};
    fontMaker.addFont(maze::assets::liberation_mono_ttf(), 20);
    app->uiRenderer()->setFontCollection(fontMaker.compile());

    resources_.playerShotSoundData = app->audio()->loadOGG(maze::assets::shoot_ogg());
    resources_.enemyShotSoundData = app->audio()->loadOGG(maze::assets::enemy_shoot_ogg());
    resources_.explosionSoundData = app->audio()->loadOGG(maze::assets::explode_ogg());
    resources_.laserHitWallSoundData = app->audio()->loadOGG(maze::assets::laser_hit_wall_ogg());
}
