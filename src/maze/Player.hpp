// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Input.hpp"
#include "Macros.hpp"
#include "MazeComponents.hpp"
#include "Timer.hpp"
#include <entt/fwd.hpp>

class GameStage;

class Player
{
public:
    Player(GameStage* gameStage, NavIndex startSector, float startOrientation);
    ~Player();

    entt::entity entity() const { return entity_; }

    void handleInputEvents(ngn::InputAction action, int key, ngn::InputMods mods);
    void update(float deltaTime);

    void kill();
    void reset();

private:
    void handleInput(float deltaTime);
    void handleBoosterAction(bool shouldBeActive);

private:
    GameStage* gameStage_;
    entt::registry* registry_;
    NavIndex startSector_;
    float startOrientation_;
    entt::entity entity_{};
    entt::entity boosterEntity_{};
    ngn::Timer laserReloadTimer_{};
    bool boosterActive_{};

    NGN_DISABLE_COPY_MOVE(Player)
};
