// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "GameStage.hpp"
#include "Enemies.hpp"
#include "Explosions.hpp"
#include "Level.hpp"
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
    level_{},
    enemies_{},
    shots_{},
    explosions_{},
    playerGameState_{},
    halfViewSize_{},
    playerViewBounds_{}
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    ,debugShowBodies_{false}
    ,debugShowBoundingBoxes_{false}
#endif
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

    delete level_;
    level_ = new Level{app_};

    ActorCreateInfo createInfo{
        .position = {96, 96},
        .rotation = glm::pi<float>(),
        .sprite = {
            .texCoords = {0, 0, 38, 40},
            .size = {39, 41},
            .texture = 1,
        },
        .body = {
            .invMass = 1.f / 10.f,
            .restitution = 1.5f,
        },
        .shape = ngn::Shape{ngn::Circle{.center = {0, 2}, .radius = 17}},
    };
    playerGameState_.entity = createActor(createInfo);
    registry_->emplace<PlayerTag>(playerGameState_.entity);

    enemies_ = new Enemies{this};
    enemies_->createEnemy({352, 352}, 0.0f);

    shots_ = new Shots{this};

    explosions_ = new Explosions{this};
}

void GameStage::onDeactivate()
{
    delete explosions_;

    delete shots_;

    delete enemies_;

    delete level_;

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

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    if (action == ngn::InputAction::Press)
    {
        if (ngn::inputModsSet(mods, ngn::InputMods::Alt) && key == GLFW_KEY_P)
        {
            debugShowBodies_ = !debugShowBodies_;
        }
        else if (ngn::inputModsSet(mods, ngn::InputMods::Alt) && key == GLFW_KEY_B)
        {
            debugShowBoundingBoxes_ = !debugShowBoundingBoxes_;
        }
    }
#endif
}

void GameStage::onUpdate(float deltaTime)
{
    handlePlayerInput(deltaTime);

    // ****************************************************

    enemies_->update(deltaTime);

    // ****************************************************

    shots_->update(deltaTime);

    // ****************************************************

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

    app_->uiRenderer()->writeText(0, "Hello Maze ][", 10, 25);

    // ****************************************************

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    app_->debugRenderer()->updateView(playerView);

    app_->world()->debugDrawState(app_->debugRenderer(), debugShowBodies_, debugShowBoundingBoxes_, false, true);
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
            playerGameState_.laserReloadTimer.setZero();
        }
    }
}

void GameStage::handlePlayerInput(float deltaTime)
{
    NGN_UNUSED(deltaTime);

    if (app_->isKeyDown(GLFW_KEY_LEFT))
    {
        auto& force = registry_->get<ngn::AngularForce>(playerGameState_.entity).value;
        force += 20.f;
    }

    if (app_->isKeyDown(GLFW_KEY_RIGHT))
    {
        auto& force = registry_->get<ngn::AngularForce>(playerGameState_.entity).value;
        force -= 20.f;
    }
    if (app_->isKeyDown(GLFW_KEY_UP))
    {
        auto [force, rot] = registry_->get<ngn::LinearForce, const ngn::Rotation>(playerGameState_.entity);
        force.value -= rot.dir * 2000.f;
    }
    if (app_->isKeyDown(GLFW_KEY_DOWN))
    {
        auto [force, rot] = registry_->get<ngn::LinearForce, const ngn::Rotation>(playerGameState_.entity);
        force.value += rot.dir * 2000.f;
    }
    if (app_->isKeyDown(GLFW_KEY_W))
    {
        auto& force = registry_->get<ngn::LinearForce>(playerGameState_.entity);
        force.value += glm::vec2{0, -2000.f};
    }
    if (app_->isKeyDown(GLFW_KEY_S))
    {
        auto& force = registry_->get<ngn::LinearForce>(playerGameState_.entity);
        force.value += glm::vec2{0, 2000.f};
    }
    if (app_->isKeyDown(GLFW_KEY_A))
    {
        auto& force = registry_->get<ngn::LinearForce>(playerGameState_.entity);
        force.value += glm::vec2{-2000.f, 0};
    }
    if (app_->isKeyDown(GLFW_KEY_D))
    {
        auto& force = registry_->get<ngn::LinearForce>(playerGameState_.entity);
        force.value += glm::vec2{2000.f, 0};
    }

    // ****************************************************

    if (app_->isKeyDown(GLFW_KEY_SPACE))
    {
        if (playerGameState_.laserReloadTimer.elapsed(ngn::Duration<double>(0.5)).first)
        {
            auto [pos, rot] = registry_->get<const ngn::Position, const ngn::Rotation>(playerGameState_.entity);
            const auto start = pos.value - rot.dir * 20.0f;
            shots_->fireLaser(start, rot.angle, true);
        }
    }
}
