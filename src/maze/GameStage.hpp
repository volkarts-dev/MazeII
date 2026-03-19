// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Timer.hpp"
#include "gfx/GfxComponents.hpp"
#include "phys/Shapes.hpp"
#include "phys/World.hpp"
#include "Application.hpp"

class Enemies;
class Explosions;
class KeyboardHandler;
class Board;
class MazeDelegate;
class Player;
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

class GameStage : public ngn::ApplicationStage
{
private:
    enum class Pause : uint32_t
    {
        Off,
        Init,
        On,
    };

public:
    enum class State : uint32_t
    {
        Active,
        Inactive,
    };

public:
    GameStage(MazeDelegate* delegate);
    ~GameStage() override;

    ngn::Application* app() const { return app_; }
    Board* board() const { return board_; }
    Shots* shots() const { return shots_; }
    Explosions* explosions() const { return explosions_; }

    void onActivate() override;
    void onDeactivate() override;

    void onWindowResize(const glm::vec2& windowSize) override;
    void onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods) override;

    void onUpdate(float deltaTime) override;
    void onDraw(float deltaTime) override;

    const Resources& resources() const;

    entt::entity createActor(const ActorCreateInfo& createInfo);

    bool testInSight(const glm::vec2& pos);

    void killEnemy(entt::entity enemy);
    void killPlayer();

    bool pause() const { return pause_ != Pause::Off; }
    void cyclePause();
    State state() const { return state_; }

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    bool debugShowBodies() const { return debugShowBodies_; }
    bool debugShowBoundingBoxes() const { return debugShowBoundingBoxes_; }
    bool debugShowAIStates() const { return debugShowAIStates_; }
#endif

private:
    void updateProjections();
    void handleAllEnemiesDown();
    void resetGame();

private:
    MazeDelegate* delegate_;
    ngn::Application* app_;
    entt::registry* registry_;
    Board* board_;
    Player* player_;
    Enemies* enemies_;
    Shots* shots_;
    Explosions* explosions_;

    entt::connection allEnemiesDownConn_;
    glm::vec2 halfViewSize_;
    glm::vec4 playerViewBounds_;
    uint32_t level_;
    uint32_t newLevel_;
    Pause pause_;
    State state_;

    float zoom_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    bool debugShowBodies_{false};
    bool debugShowBoundingBoxes_{false};
    bool debugShowAIStates_{false};
#endif
};
