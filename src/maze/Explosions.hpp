// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "phys/World.hpp"
#include "Macros.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

class GameStage;

class Explosions
{
public:
    enum class Type
    {
        One,
    };

public:
    Explosions(GameStage* gameStage);
    ~Explosions();

    void showExplosion(const glm::vec2& position, Type type);

    void update(float deltaTime);

private:
    GameStage* gameStage_;
    entt::registry* registry_;
    ngn::World* world_;

    NGN_DISABLE_COPY_MOVE(Explosions)
};
