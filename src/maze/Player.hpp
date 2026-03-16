// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "Input.hpp"
#include "Macros.hpp"
#include "Timer.hpp"
#include <entt/fwd.hpp>

namespace ngn {
} // namespace ngn

class GameStage;

class Player
{
public:
    Player(GameStage* gameStage);
    ~Player();

    const glm::vec2& position() const;

    void handleInputEvents(ngn::InputAction action, int key, ngn::InputMods mods);
    void update(float deltaTime);

    void kill();
    void reset();

private:
    void handleInput(float deltaTime);

private:
    GameStage* gameStage_;
    entt::registry* registry_;
    entt::entity entity_{};
    entt::entity boosterEntity_{};
    ngn::Timer laserReloadTimer_{};

    NGN_DISABLE_COPY_MOVE(Player)
};
