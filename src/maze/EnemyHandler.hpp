// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "phys/Shapes.hpp"
#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

namespace ngn {
class Application;
class World;
} // namespace ngn

class GameStage;

class EnemyHandler
{
public:
    EnemyHandler(GameStage* gameStage);
    ~EnemyHandler();

    void createEnemy(glm::vec2 pos, float angle);

    void update(float deltaTime);

private:
    enum class State
    {
        Idle,
        Persuit,
        Evasion,
    };

    class EnemyInfo
    {
    public:
        State state{State::Idle};
    };

private:
    bool testInSight(entt::entity player, entt::entity enemy, const ngn::Line& lineOfSight);

private:
    GameStage* gameStage_;
    entt::registry* registry_;
    ngn::World* world_;
    float updateTimer_;

    NGN_DISABLE_COPY_MOVE(EnemyHandler)
};
