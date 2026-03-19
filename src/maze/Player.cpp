// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "Player.hpp"

#include "Board.hpp"
#include "Explosions.hpp"
#include "GameStage.hpp"
#include "Layers.hpp"
#include "MazeComponents.hpp"
#include "Shots.hpp"
#include "ai/NavigationGraph.hpp"
#include "gfx/SpriteAnimator.hpp"
#include "phys/PhysComponents.hpp"
#include <entt/entt.hpp>

Player::Player(GameStage* gameStage) :
    gameStage_{gameStage},
    registry_{gameStage_->app()->registry()}
{
    ActorCreateInfo createInfo{
        .position = gameStage_->board()->navigationGraph()->midPoint(10),
        .rotation = 0.0f,
        .sprite = {
            .texCoords = {0, 0, 38, 40},
            .size = {39, 41},
            .texture = 1,
        },
        .body = {
            .layers = LayerPlayer,
            .invMass = 1.f / 10.f,
            .restitution = 1.5f,
        },
        .shape = ngn::Shape{ngn::Circle{.center = {0, 2}, .radius = 17}},
    };
    entity_ = gameStage_->createActor(createInfo);
    registry_->emplace<PlayerTag>(entity_);

    //boosterEntity_ = registry_->create();
    //registry_->emplace<ngn::Position>(boosterEntity_);
    //registry_->emplace<ngn::Sprite>(boosterEntity_, ngn::Sprite{
    //    .texCoords = {88, 0, 100, 10},
    //    .size{1, 1},
    //    .texture = 1,
    //});
    //ngn::SpriteAnimationBuilder animationBuilder{};
    //animationBuilder
    //    .addFrame(glm::vec4{88, 0, 102, 11}, 1, 0.1f)
    //    .addFrame(glm::vec4{88, 12, 102, 23}, 1, 0.1f)
    //    .addFrame(glm::vec4{103, 0, 117, 11}, 1, 0.1f)
    //    .addFrame(glm::vec4{103, 12, 117, 23}, 1, 0.1f)
    //    .setRepeat(true)
    //    ;
    //gameStage_->app()->spriteAnimationHandler()->createAnimation(boosterEntity_, animationBuilder);
}

Player::~Player()
{
    //registry_->destroy(boosterEntity_);
    registry_->destroy(entity_);
}

const glm::vec2& Player::position() const
{
    return registry_->get<const ngn::Position>(entity_).value;
}

void Player::update(float deltaTime)
{
    laserReloadTimer_.update(deltaTime);
    handleInput(deltaTime);
}

void Player::kill()
{
    gameStage_->explosions()->doExplosion(position(), Explosions::Type::One);

    registry_->remove<ngn::ActiveTag>(entity_);

    //gameStage_->app()->spriteAnimationHandler()->stopAnimation(boosterEntity_);
}

void Player::reset()
{
    auto [pos, rot, linVel, angVel, linFor, angFor] = registry_->get<
        ngn::Position,
        ngn::Rotation,
        ngn::LinearVelocity,
        ngn::AngularVelocity,
        ngn::LinearForce,
        ngn::AngularForce>(entity_);
    pos.value = gameStage_->board()->navigationGraph()->midPoint(10);
    rot.angle = 0.0f;
    rot.update();
    linVel.value = {};
    angVel.value = {};
    linFor.value = {};
    angFor.value = {};
    registry_->emplace<ngn::TransformChangedTag>(entity_);
    registry_->emplace_or_replace<ngn::ActiveTag>(entity_);
}

void Player::handleInputEvents(ngn::InputAction action, int key, ngn::InputMods mods)
{
    NGN_UNUSED(mods);

    if (gameStage_->state() == GameStage::State::Active)
    {
        if (action == ngn::InputAction::Press)
        {
            if (key == GLFW_KEY_SPACE)
            {
                laserReloadTimer_.restart(true);
            }
        }
    }
}

void Player::handleInput(float deltaTime)
{
    NGN_UNUSED(deltaTime);

    if (gameStage_->state() == GameStage::State::Active)
    {
        auto* app = gameStage_->app();

        if (app->isKeyDown(GLFW_KEY_LEFT))
        {
            auto& force = registry_->get<ngn::AngularForce>(entity_).value;
            force += 20.0f;
        }
        if (app->isKeyDown(GLFW_KEY_RIGHT))
        {
            auto& force = registry_->get<ngn::AngularForce>(entity_).value;
            force -= 20.0f;
        }
        if (app->isKeyDown(GLFW_KEY_UP))
        {
            const auto factor = app->isKeyDown(GLFW_KEY_Q) ? 7000.0f : 2000.0f;
            auto [force, rot] = registry_->get<ngn::LinearForce, const ngn::Rotation>(entity_);
            force.value += rot.dir * factor;
        }
        if (app->isKeyDown(GLFW_KEY_DOWN))
        {
            auto [force, rot] = registry_->get<ngn::LinearForce, const ngn::Rotation>(entity_);
            force.value -= rot.dir * 2000.0f;
        }

        // ****************************************************

        if (app->isKeyDown(GLFW_KEY_SPACE))
        {
            if (laserReloadTimer_.isElapsed(0.5f).first)
            {
                auto [pos, rot] = registry_->get<const ngn::Position, const ngn::Rotation>(entity_);
                const auto start = pos.value + rot.dir * 20.0f;
                gameStage_->shots()->fireLaser(start, rot.angle, true);
            }
        }
    }
}
