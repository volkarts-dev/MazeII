// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "MazeComponents.hpp"
#include "Timer.hpp"
#include "phys/Shapes.hpp"
#include <entt/fwd.hpp>
#include <glm/fwd.hpp>
#include <random>

namespace ngn {
class Application;
class World;
} // namespace ngn

class GameStage;
class Level;

class Enemies
{
public:
    Enemies(GameStage* gameStage);
    ~Enemies();

    void createEnemy(NavIndex startSector, float angle);
    void killEnemy(entt::entity enemy);

    void update(float deltaTime);

private:
    enum class State
    {
        Idle,
        Persuit,
        Evasion,
        Wander,
    };

    class EnemyInfo
    {
    public:
        State state{State::Idle};
    };

private:
    NavIndex findNextRandomSector(NavIndex last, NavIndex current);
    bool testInSight(entt::entity player, entt::entity enemy, const ngn::Line& lineOfSight);

private:
    GameStage* gameStage_;
    entt::registry* registry_;
    ngn::World* world_;
    ngn::NavigationGraph* navigationGraph_;
    ngn::Timer updateTimer_;

    std::mt19937 randGenerator_;

    NGN_DISABLE_COPY_MOVE(Enemies)
};
