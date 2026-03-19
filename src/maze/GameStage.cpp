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
#include "gfx/UiRenderer.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "phys/World.hpp"
#include <GLFW/glfw3.h>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

GameStage::GameStage(MazeDelegate* delegate) :
    delegate_{delegate},
    app_{delegate_->app()},
    registry_{app_->registry()},
    board_{},
    player_{},
    enemies_{},
    shots_{},
    explosions_{},
    halfViewSize_{},
    playerViewBounds_{},
    level_{1},
    pause_{},
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

    board_ = new Board{app_};

    player_ = new Player{this};

    enemies_ = new Enemies{this};
    allEnemiesDownConn_ = enemies_->addAllEnemiesDownListener<&GameStage::handleAllEnemiesDown>(this);
    enemies_->createEnemy(41, ngn::math::PI);
    enemies_->createEnemy(43, ngn::math::PI);
    //enemies_->createEnemy(320, 0.0f);
    //enemies_->createEnemy(322, 0.0f);
    //enemies_->createEnemy(324, 0.0f);
    //enemies_->createEnemy(326, 0.0f);
    //enemies_->createEnemy(328, 0.0f);
    //enemies_->createEnemy(332, 0.0f);
    //enemies_->createEnemy(334, 0.0f);
    //enemies_->createEnemy(336, 0.0f);
    //enemies_->createEnemy(338, 0.0f);
    //enemies_->createEnemy(340, 0.0f);

    shots_ = new Shots{this};

    explosions_ = new Explosions{this};

    pause_ = Pause::On;
    state_ = State::Active;
}

void GameStage::onDeactivate()
{
    delete explosions_;

    delete shots_;

    allEnemiesDownConn_.release();
    delete enemies_;

    delete player_;

    delete board_;
}

void GameStage::onWindowResize(const glm::vec2& windowSize)
{
    NGN_UNUSED(windowSize);

    updateProjections();
}

void GameStage::onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods)
{
    player_->handleInputEvents(action, key, mods);

    if (action == ngn::InputAction::Press)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            app_->quit();
            return;
        }
        else if (mods == ngn::InputMods::Ctrl && key == GLFW_KEY_PAGE_UP)
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

#if !defined(NGN_ENABLE_INSTRUMENTATION)
        if (!pause() && key == GLFW_KEY_P)
        {
            cyclePause();
        }
#endif
    }
    else if (action == ngn::InputAction::Release)
    {
#if !defined(NGN_ENABLE_INSTRUMENTATION)
        if (pause() && (key == GLFW_KEY_P || key == GLFW_KEY_SPACE))
        {
            cyclePause();
        }
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
    if (pause())
        return;

    if (state_ == State::Inactive)
    {
        if (explosions_->allDone())
            resetGame();
    }
    // ****************************************************

    player_->update(deltaTime);

    // ****************************************************

    enemies_->update(deltaTime);

    // ****************************************************

    shots_->update(deltaTime);
}

void GameStage::onDraw(float deltaTime)
{
    NGN_UNUSED(deltaTime);

    const auto playerPos = player_->position();

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

    const auto pauseInfo = pause() ? " - Pause (P or Space to start)"sv : ""sv;
    const auto levelInfo = fmt::format("Maze ][ - Lvl:{}{}", level_, pauseInfo);
    app_->uiRenderer()->renderText(ngn::FontId{0}, levelInfo, 10, 25);

    // ****************************************************

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    enemies_->debugDraw();

    app_->debugRenderer()->updateView(playerView);

    app_->world()->debugDrawState(app_->debugRenderer(), debugShowBodies_, debugShowBoundingBoxes_, false, debugShowBodies_);
    if (debugShowAIStates_)
        board_->debugDrawState(app_->debugRenderer());
#endif
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

void GameStage::killEnemy(entt::entity enemy)
{
    enemies_->killEnemy(enemy);
}

void GameStage::killPlayer()
{
    player_->kill();

    newLevel_ = 1;
    state_ = State::Inactive;
}

void GameStage::cyclePause()
{
    switch (pause_)
    {
        case Pause::Off:  pause_ = Pause::Init; break;
        case Pause::Init: pause_ = Pause::On; break;
        case Pause::On:   pause_ = Pause::Off; break;
    }
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

void GameStage::handleAllEnemiesDown()
{
    newLevel_ = level_ + 1;
    state_ = State::Inactive;
}

void GameStage::resetGame()
{
    player_->reset();
    enemies_->reset();
    shots_->reset();
    level_ = newLevel_;
    pause_ = Pause::On;
    state_ = State::Active;
}
