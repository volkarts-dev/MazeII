// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Application.hpp"
#include "gfx/GfxComponents.hpp"
#include "gfx/GfxIds.hpp"
#include "phys/Shapes.hpp"
#include "phys/World.hpp"

namespace ngn {
class Sampler;
} // namespace ngn

class Dialog;
class Enemies;
class Explosions;
class KeyboardHandler;
class Board;
class MazeDelegate;
class OverviewMap;
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
public:
    enum class State : uint32_t
    {
        Active,
        LevelEnded,
    };

public:
    GameStage(MazeDelegate* delegate);
    ~GameStage() override;

    ngn::Application* app() const { return app_; }
    MazeDelegate* delegate() const { return delegate_; }
    Board* board() const { return board_; }
    Shots* shots() const { return shots_; }
    Explosions* explosions() const { return explosions_; }
    uint32_t level() const { return level_; }

    void onActivate() override;
    void onDeactivate() override;

    void onWindowResize(const glm::vec2& windowSize) override;
    void onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods) override;

    void onUpdate(float deltaTime) override;
    void onDraw(float deltaTime) override;
    void onCustomRenderPasses(ngn::CommandBuffer* commandBuffer) override;

    const Resources& resources() const;

    entt::entity createActor(const ActorCreateInfo& createInfo);

    bool testInSight(const glm::vec2& pos);

    void killEnemy(entt::entity enemy);
    void killPlayer();

    State state() const { return state_; }

    void triggerNormalQuit();

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

    Dialog* dialog_;
    Board* board_;
    Player* player_;
    Enemies* enemies_;
    Shots* shots_;
    Explosions* explosions_;
    OverviewMap* overviewMap_;
    ngn::TextureId overviewMapTexture_;

    entt::connection allEnemiesDownConn_;
    glm::vec2 halfViewSize_;
    glm::vec4 playerViewBounds_;
    uint32_t points_;
    uint32_t newPoints_;
    uint32_t level_;
    uint32_t newLevel_;
    State state_;

    float zoom_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    bool debugShowBodies_{false};
    bool debugShowBoundingBoxes_{false};
    bool debugShowAIStates_{false};
#endif
};
