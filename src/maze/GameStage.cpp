// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "GameStage.hpp"
#include "Enemies.hpp"
#include "Explosions.hpp"
#include "Board.hpp"
#include "Math.hpp"
#include "MazeDelegate.hpp"
#include "Player.hpp"
#include "Shots.hpp"
#include "gfx/CommandBuffer.hpp"
#include "gfx/Dialog.hpp"
#include "gfx/Image.hpp"
#include "gfx/OverviewMap.hpp"
#include "gfx/UiRenderer.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "phys/World.hpp"
#include <entt/entt.hpp>
#include <GLFW/glfw3.h>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

GameStage::GameStage(MazeDelegate* delegate) :
    delegate_{delegate},
    app_{delegate_->app()},
    registry_{app_->registry()},
    dialog_{},
    board_{},
    player_{},
    enemies_{},
    shots_{},
    explosions_{},
    overviewMap_{},
    halfViewSize_{},
    playerViewBounds_{},
    level_{1},
    state_{},
    zoom_{1.0f}
{
}

GameStage::~GameStage()
{
}

void GameStage::onActivate()
{
    app_->world()->setConfig({
        .linearDamping = 1.0f,
        .angularDamping = 1.0f,
        .gravity{},
    });

    // ****************************************************

    dialog_ = new Dialog{this};

    // ****************************************************

    board_ = new Board{app_};

    // ****************************************************

    player_ = new Player{this, 126, ngn::math::PI};

    // ****************************************************

    enemies_ = new Enemies{this};
    allEnemiesDownConn_ = enemies_->addAllEnemiesDownListener<&GameStage::handleAllEnemiesDown>(this);

    // TEMP
    //enemies_->createEnemy(25, ngn::math::PI);
    //enemies_->createEnemy(27, ngn::math::PI);

    // BIG
    //enemies_->createEnemy(0, 0.0f);
    //enemies_->createEnemy(2, 0.0f);
    //enemies_->createEnemy(4, 0.0f);
    //enemies_->createEnemy(6, 0.0f);
    //enemies_->createEnemy(8, 0.0f);
    //enemies_->createEnemy(12, 0.0f);
    //enemies_->createEnemy(14, 0.0f);
    //enemies_->createEnemy(16, 0.0f);
    //enemies_->createEnemy(18, 0.0f);
    //enemies_->createEnemy(10, 0.0f);

    // SMALL
    enemies_->createEnemy(40, 0.0f);
    enemies_->createEnemy(20, 0.0f);
    enemies_->createEnemy(0, 0.0f);
    enemies_->createEnemy(2, 0.0f);
    enemies_->createEnemy(4, 0.0f);
    enemies_->createEnemy(8, 0.0f);
    enemies_->createEnemy(10, 0.0f);
    enemies_->createEnemy(12, 0.0f);
    enemies_->createEnemy(32, 0.0f);
    enemies_->createEnemy(52, 0.0f);

    // ****************************************************

    shots_ = new Shots{this};

    // ****************************************************

    explosions_ = new Explosions{this};

    // ****************************************************

    overviewMap_ = new OverviewMap{this, glm::u32vec2{100, 100}, 16};

    overviewMapTexture_ = app_->uiRenderer()->reserveTexture();

    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        app_->uiRenderer()->updateSamplerDescriptor(overviewMapTexture_, f, overviewMap_->mapImageView(f));
    }

    // ****************************************************

    state_ = State::Active;

    // ****************************************************

#if !defined(NGN_ENABLE_INSTRUMENTATION)
    DialogData data{
        .size = {256, 256},
        .title = "Welcome",
        .text = "Welcome to Maze ][",
        .button1 = "Start",
        .button2 = "Quit",
        .defaultButton = DialogButton::One,
    };
    data.button2Callback.connect<&GameStage::triggerNormalQuit>(this);
    dialog_->show(data);
#endif
}

void GameStage::onDeactivate()
{
    delete overviewMap_;

    delete explosions_;

    delete shots_;

    allEnemiesDownConn_.release();
    delete enemies_;

    delete player_;

    delete board_;

    delete dialog_;
}

void GameStage::onWindowResize(const glm::vec2& windowSize)
{
    NGN_UNUSED(windowSize);

    updateProjections();
}

void GameStage::onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods)
{
#if !defined(NGN_ENABLE_INSTRUMENTATION)
    if (dialog_->handleInputEvents(action, key, mods))
        return;

    if (state_ == GameStage::State::Active)
    {
        player_->handleInputEvents(action, key, mods);
    }

    if (action == ngn::InputAction::Press)
    {
        if (mods == ngn::InputMods::Ctrl && key == GLFW_KEY_PAGE_UP)
        {
            zoom_ = glm::min(zoom_ + 0.5f, 5.0f);
            updateProjections();
        }
        else if (mods == ngn::InputMods::Ctrl && key == GLFW_KEY_PAGE_DOWN)
        {
            zoom_ = glm::max(zoom_ - 0.5f, 0.5f);
            updateProjections();
        }
        else if (mods == ngn::InputMods::Ctrl && key == GLFW_KEY_0)
        {
            zoom_ = 1.0f;
            updateProjections();
        }
        else if (key == GLFW_KEY_F1)
        {
            DialogData data{
                .size = {256, 256},
                .title = "Help",
                .text = "Key Bindings:\n[Arrows]: Move\n[Space]: Fire\nP: Pause\nF1: Help",
                .button1 = "OK",
                .defaultButton = DialogButton::One,
            };
            dialog_->show(data);
            return;
        }
        else if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_P)
        {
            DialogData data{
                .size = {256, 256},
                .title = "Pause",
                .text = "",
                .button1 = "Resume",
                .button2 = "Quit",
                .defaultButton = DialogButton::One,
            };
            data.button2Callback.connect<&GameStage::triggerNormalQuit>(this);
            dialog_->show(data);
            return;
        }

#if defined(NGN_ENABLE_DEVELOPER_HACKS)
        if (mods == ngn::InputMods::Ctrl && key == GLFW_KEY_K)
        {
            auto view = registry_->view<EnemyTag, ngn::ActiveTag>();
            for (const auto e : view)
            {
                killEnemy(e);
            }
        }
#endif
#endif
    }

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    if (action == ngn::InputAction::Press)
    {
        if (ngn::inputModsSet(mods, ngn::InputMods::Ctrl | ngn::InputMods::Shift) && key == GLFW_KEY_1)
        {
            debugShowBoundingBoxes_ = !debugShowBoundingBoxes_;
        }
        else if (ngn::inputModsSet(mods, ngn::InputMods::Ctrl | ngn::InputMods::Shift) && key == GLFW_KEY_2)
        {
            debugShowBodies_ = !debugShowBodies_;
        }
        else if (ngn::inputModsSet(mods, ngn::InputMods::Ctrl | ngn::InputMods::Shift) && key == GLFW_KEY_3)
        {
            debugShowAIStates_ = !debugShowAIStates_;
        }
    }
#endif
}

void GameStage::onUpdate(float deltaTime)
{
    if (dialog_->isFinished())
    {
        // callbacks already fired
        dialog_->reset();
    }
    else if (dialog_->isActive())
    {
        dialog_->update(deltaTime);
        return;
    }

    if (state_ == State::LevelEnded)
    {
        if (explosions_->allDone())
        {
            DialogData data{
                .size = {256, 256},
            };
            if (newLevel_ == 1)
            {
                data.title = "Game over";
                data.text = "Ready to restart";
                data.button1 = "Restart";
                data.button2 = "Quit";
                data.defaultButton = DialogButton::One;
            }
            else
            {
                data.title = "Excellent";
                data.text = "Reached next level";
                data.button1 = "Continue";
                data.button2 = "Quit";
                data.defaultButton = DialogButton::One;
            }
            data.button1Callback.connect<&GameStage::resetGame>(this);
            data.button2Callback.connect<&GameStage::triggerNormalQuit>(this);
            dialog_->show(data);
        }
        return;
    }

    player_->update(deltaTime);

    enemies_->update(deltaTime);

    shots_->update(deltaTime);
}

void GameStage::onDraw(float deltaTime)
{
    NGN_UNUSED(deltaTime);

    const auto playerPos = registry_->get<const ngn::Position>(player_->entity()).value;

    playerViewBounds_ = {
        playerPos - halfViewSize_,
        playerPos + halfViewSize_,
    };

    const auto playerView = glm::lookAt(
        glm::vec3{playerPos, 0.5f},
        glm::vec3{playerPos, 0.0f},
        glm::vec3{0.0f, 1.0f, 0.0f}
    );

    // ****************************************************

    app_->spriteRenderer()->updateView(playerView);

    app_->spriteRenderer()->renderSpriteComponents(registry_);

    // ****************************************************

    overviewMap_->renderPoints();

    // ****************************************************

    const auto levelInfo = fmt::format("Maze ][ - Lvl:{}", level_);
    app_->uiRenderer()->renderText(ngn::FontId{0}, levelInfo, {10, 25});

    app_->uiRenderer()->renderSprite({
        .position = glm::vec2{1024 - 60, 60},
        .rotation = 0.0f,
        .scale = glm::vec2{100, 100},
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
        .texCoords = glm::vec4{0, 0, 100, 100},
        .texIndex = static_cast<uint32_t>(overviewMapTexture_),
    });

    // ****************************************************

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    enemies_->debugDraw();

    app_->debugRenderer()->updateView(playerView);

    app_->world()->debugDrawState(app_->debugRenderer(), debugShowBodies_, debugShowBoundingBoxes_, false, debugShowBodies_);
    if (debugShowAIStates_)
        board_->debugDrawState(app_->debugRenderer());
#endif

    // ****************************************************

    dialog_->draw();
}

void GameStage::onCustomRenderPasses(ngn::CommandBuffer* commandBuffer)
{
    commandBuffer->beginRenderPass(overviewMap_->renderTarget(), app_->renderer()->currentFrame());

    overviewMap_->draw(commandBuffer);

    commandBuffer->endRenderPass();
}

const Resources& GameStage::resources() const
{
    return delegate_->resources();
}

entt::entity GameStage::createActor(const ActorCreateInfo& createInfo)
{
    auto entity = app_->createActor(createInfo.position, createInfo.rotation, createInfo.scale, createInfo.active);
    app_->world()->createBody(entity, createInfo.body, createInfo.shape);
    registry_->emplace<ngn::Sprite>(entity, createInfo.sprite);
    return entity;
}

bool GameStage::testInSight(const glm::vec2& pos)
{
    return
        (pos.x >= playerViewBounds_.x) & (pos.y >= playerViewBounds_.y) &
        (pos.x <= playerViewBounds_.z) & (pos.y <= playerViewBounds_.w);
}

void GameStage::triggerNormalQuit()
{
    app_->quit(0);
}

void GameStage::updateProjections()
{
    const auto windowSize = app_->windowSize();

    halfViewSize_ = (windowSize + 50.0f) * 0.5f;

    const auto halfSize = windowSize * 0.5f * zoom_;
    const auto proj = glm::ortho(
        -halfSize.x, halfSize.x,
        -halfSize.y, halfSize.y,
        -1.0f, 1.0f
    );

    const auto uiHalfSize = windowSize * 0.5f;
    const auto uiProj = glm::ortho(
        -uiHalfSize.x, uiHalfSize.x,
        -uiHalfSize.y, uiHalfSize.y,
        -1.0f, 1.0f
    );

    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        app_->spriteRenderer()->updateProj(proj, i);

        app_->uiRenderer()->updateView(glm::lookAt(
            glm::vec3{uiHalfSize, 0.5f},
            glm::vec3{uiHalfSize, 0.0f},
            glm::vec3{0.0f, 1.0f, 0.0f}
        ), i);
        app_->uiRenderer()->updateProj(uiProj, i);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        app_->debugRenderer()->updateProj(proj, i);
#endif
    }
}

void GameStage::killEnemy(entt::entity enemy)
{
    enemies_->killEnemy(enemy);
}

void GameStage::killPlayer()
{
    player_->kill();

    newLevel_ = 1;
    state_ = State::LevelEnded;
}

void GameStage::handleAllEnemiesDown()
{
    newLevel_ = level_ + 1;
    state_ = State::LevelEnded;
}

void GameStage::resetGame()
{
    player_->reset();
    enemies_->reset();
    shots_->reset();
    level_ = newLevel_;
    state_ = State::Active;
}
