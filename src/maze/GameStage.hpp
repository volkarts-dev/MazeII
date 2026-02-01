// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Timer.hpp"
#include "gfx/GFXComponents.hpp"
#include "phys/Shapes.hpp"
#include "phys/World.hpp"
#include "Application.hpp"

class Enemies;
class Explosions;
class KeyboardHandler;
class Level;
class MazeDelegate;
class Resources;
class Shots;

class ActorCreateInfo
{
public:
    glm::vec2 position{};
    float rotation{};
    glm::vec2 scale{1, 1};
    ngn::Sprite sprite{};
    ngn::BodyCreateInfo body{};
    ngn::Shape shape{};
    bool active{true};
};

class PlayerGameState
{
public:
    ngn::Timer laserReloadTimer;
    entt::entity entity;
};

class GameStage : public ngn::ApplicationStage
{
public:
    GameStage(MazeDelegate* delegate);
    ~GameStage() override;

    ngn::Application* app() const { return app_; }

    void onActivate() override;
    void onDeactivate() override;

    void onWindowResize(const glm::vec2& windowSize) override;
    void onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods) override;

    void onUpdate(float deltaTime) override;

    const Resources& resources() const;

    entt::entity createActor(const ActorCreateInfo& createInfo);

    void killEnemy(entt::entity enemy);

private:
    void handlePlayerInputEvents(ngn::InputAction action, int key, ngn::InputMods mods);
    void handlePlayerInput(float deltaTime);

private:
    MazeDelegate* delegate_;
    ngn::Application* app_;
    entt::registry* registry_;
    Level* level_;
    Enemies* enemies_;
    Shots* shots_;
    Explosions* explosions_;

    PlayerGameState playerGameState_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    bool debugShowBodies_;
    bool debugShowBoundingBoxes_;
#endif
};
