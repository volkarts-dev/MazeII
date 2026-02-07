// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "phys/World.hpp"
#include "Macros.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

class GameStage;

class Shots
{
public:
    Shots(GameStage* gameStage);
    ~Shots();

    void fireLaser(const glm::vec2& position, float rotation, bool player);

    void update(float deltaTime);

    void handleCollision(const ngn::Collision& collision);

private:
    GameStage* gameStage_;
    entt::registry* registry_;
    ngn::World* world_;
    entt::connection collisionCallback_;

    NGN_DISABLE_COPY_MOVE(Shots)
};
