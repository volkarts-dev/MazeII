// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "MazeComponents.hpp"
#include "Timer.hpp"
#include <entt/signal/sigh.hpp>
#include <glm/fwd.hpp>
#include <random>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "phys/Collision.hpp"
#endif

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

    template<auto Callback, typename... Args>
    entt::connection addAllEnemiesDownListener(Args&&... args);

    void createEnemy(NavIndex startSector, float angle);
    void killEnemy(entt::entity enemy);

    void reset();

    void update(float deltaTime);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    void debugDraw();
#endif

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

    class FindObstacleResult
    {
    public:
        glm::vec2 dir{};
        float depth{};
        bool found{};
    };

private:
    NavIndex findNextRandomSector(NavIndex last, NavIndex current);
    bool testInSight(const glm::vec2& origin, const glm::vec2& target);
    FindObstacleResult findObstacle(entt::entity self, const glm::vec2& origin, const glm::vec2& target, float halfWidth);

private:
    GameStage* gameStage_;
    entt::registry* registry_;
    ngn::World* world_;
    ngn::NavigationGraph* navigationGraph_;
    ngn::Timer updateTimer_;

    std::mt19937 randGenerator_;

    entt::sigh<void()> allEnemiesDownSignal_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    class EnemyDebugState
    {
    public:
        ngn::Collision closestCollision;
        glm::vec2 searchWayStart;
        glm::vec2 searchWayEnd;
        FindObstacleResult obstacle;
    };


#endif

    NGN_DISABLE_COPY_MOVE(Enemies)
};

template<auto Callback, typename... Args>
inline entt::connection Enemies::addAllEnemiesDownListener(Args&&... args)
{
    entt::sink s{allEnemiesDownSignal_};
    return s.connect<Callback>(std::forward<Args>(args)...);
}
