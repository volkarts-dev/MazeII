// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "GameStage.hpp"
#include "Enemies.hpp"
#include "Explosions.hpp"
#include "Layers.hpp"
#include "Board.hpp"
#include "MazeComponents.hpp"
#include "MazeDelegate.hpp"
#include "Shots.hpp"
#include "gfx/UiRenderer.hpp"
#include "gfx/GFXComponents.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "phys/PhysComponents.hpp"
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
    enemies_{},
    shots_{},
    explosions_{},
    playerGameState_{},
    halfViewSize_{},
    playerViewBounds_{},
    level_{1},
    pause_{}
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

    ActorCreateInfo createInfo{
        .position = board()->navigationGraph()->midPoint(10),
        .rotation = glm::pi<float>(),
        .sprite = {
            .texCoords = {0, 0, 38, 40},
            .size = {39, 41},
            .texture = 1,
        },
        .body = {
            .layers = LayerPlayer,
            .invMass = 1.f / 10.f,
            .restitution = 1.5f,
        },
        .shape = ngn::Shape{ngn::Circle{.center = {0, 2}, .radius = 17}},
    };
    playerGameState_.entity = createActor(createInfo);
    registry_->emplace<PlayerTag>(playerGameState_.entity);

    enemies_ = new Enemies{this};
    allEnemiesDownConn_ = enemies_->addAllEnemiesDownListener<&GameStage::handleAllEnemiesDown>(this);
    enemies_->createEnemy(320, 0.0f);
    enemies_->createEnemy(322, 0.0f);
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

    pause_ = true;
}

void GameStage::onDeactivate()
{
    delete explosions_;

    delete shots_;

    allEnemiesDownConn_.release();
    delete enemies_;

    delete board_;

    registry_->destroy(playerGameState_.entity);
}

void GameStage::onWindowResize(const glm::vec2& windowSize)
{
    halfViewSize_ = (windowSize + 50.0f) * 0.5f;

    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        app_->uiRenderer()->updateView(glm::lookAt(
            glm::vec3{windowSize / 2.0f, 0.5f},
            glm::vec3{windowSize / 2.0f, 0.0f},
            glm::vec3{0.0f, 1.0f, 0.0f}
        ), i);
    }
}

void GameStage::onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods)
{
    handlePlayerInputEvents(action, key, mods);

    if (action == ngn::InputAction::Press)
    {
#if !defined(NGN_ENABLE_INSTRUMENTATION)
        if (key == GLFW_KEY_P)
        {
            togglePause();
        }
#endif
    }
    else if (action == ngn::InputAction::Release)
    {
#if !defined(NGN_ENABLE_INSTRUMENTATION)
        if (pause_ && key == GLFW_KEY_SPACE)
        {
            setPause(false);
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
    if (pause_)
        return;

    playerGameState_.laserReloadTimer.update(deltaTime);

    // ****************************************************

    handlePlayerInput(deltaTime);

    // ****************************************************

    enemies_->update(deltaTime);

    // ****************************************************

    shots_->update(deltaTime);
}

void GameStage::onDraw(float deltaTime)
{
    NGN_UNUSED(deltaTime);

    const auto playerPos = registry_->get<const ngn::Position>(playerGameState_.entity).value;

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

    const auto levelInfo = fmt::format("Hello Maze ][ - Lvl:{}", level_);
    app_->uiRenderer()->writeText(0, levelInfo, 10, 25);

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
    const auto& pos = registry_->get<const ngn::Position>(enemy);

    explosions_->showExplosion(pos.value, Explosions::Type::One);

    enemies_->killEnemy(enemy);
}

void GameStage::handlePlayerInputEvents(ngn::InputAction action, int key, ngn::InputMods mods)
{
    NGN_UNUSED(mods);

    if (action == ngn::InputAction::Press)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            app_->quit();
            return;
        }
        else if (key == GLFW_KEY_SPACE)
        {
            playerGameState_.laserReloadTimer.restart(true);
        }
    }
}

void GameStage::handlePlayerInput(float deltaTime)
{
    NGN_UNUSED(deltaTime);

    if (app_->isKeyDown(GLFW_KEY_LEFT))
    {
        auto& force = registry_->get<ngn::AngularForce>(playerGameState_.entity).value;
        force += 20.0f;
    }
    if (app_->isKeyDown(GLFW_KEY_RIGHT))
    {
        auto& force = registry_->get<ngn::AngularForce>(playerGameState_.entity).value;
        force -= 20.0f;
    }
    if (app_->isKeyDown(GLFW_KEY_UP))
    {
        const auto factor = app_->isKeyDown(GLFW_KEY_Q) ? 5000.0f : 2000.0f;
        auto [force, rot] = registry_->get<ngn::LinearForce, const ngn::Rotation>(playerGameState_.entity);
        force.value -= rot.dir * factor;
    }
    if (app_->isKeyDown(GLFW_KEY_DOWN))
    {
        auto [force, rot] = registry_->get<ngn::LinearForce, const ngn::Rotation>(playerGameState_.entity);
        force.value += rot.dir * 2000.0f;
    }

    // ****************************************************

    if (app_->isKeyDown(GLFW_KEY_SPACE))
    {
        if (playerGameState_.laserReloadTimer.elapsed(0.5f).first)
        {
            auto [pos, rot] = registry_->get<const ngn::Position, const ngn::Rotation>(playerGameState_.entity);
            const auto start = pos.value - rot.dir * 20.0f;
            shots_->fireLaser(start, rot.angle, true);
        }
    }
}

void GameStage::handleAllEnemiesDown()
{
    resetPlayer();
    enemies_->reset();
    level_++;
    setPause(true);
}

void GameStage::resetPlayer()
{
    auto [pos, rot, linVel, angVel, linFor, angFor] = registry_->get<
            ngn::Position,
            ngn::Rotation,
            ngn::LinearVelocity,
            ngn::AngularVelocity,
            ngn::LinearForce,
            ngn::AngularForce>(playerGameState_.entity);
    pos.value = board()->navigationGraph()->midPoint(10);
    rot.angle = glm::pi<float>();
    rot.update();
    linVel.value = {};
    angVel.value = {};
    linFor.value = {};
    angFor.value = {};
    registry_->emplace<ngn::TransformChangedTag>(playerGameState_.entity);
}
