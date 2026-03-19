// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "Player.hpp"

#include "Board.hpp"
#include "Explosions.hpp"
#include "GameStage.hpp"
#include "Layers.hpp"
#include "MazeDelegate.hpp"
#include "Shots.hpp"
#include "audio/Sound.hpp"
#include "gfx/SpriteAnimator.hpp"
#include "phys/PhysComponents.hpp"
#include <entt/entt.hpp>

Player::Player(GameStage* gameStage, NavIndex startSector, float startOrientation) :
    gameStage_{gameStage},
    registry_{gameStage_->app()->registry()},
    startSector_{startSector},
    startOrientation_{startOrientation}
{
    ActorCreateInfo createInfo{
        .position = gameStage_->board()->navigationGraph()->midPoint(startSector_),
        .rotation = startOrientation_,
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

    boosterEntity_ = registry_->create();
    registry_->emplace<ngn::Position>(boosterEntity_);
    registry_->emplace<ngn::Rotation>(boosterEntity_);
    registry_->emplace<ngn::Sprite>(boosterEntity_, ngn::Sprite{
        .texCoords = {88, 0, 100, 10},
        .size{1, 1},
        .texture = 1,
    });
    ngn::SpriteAnimationBuilder animationBuilder{};
    animationBuilder
        .addFrame(glm::vec4{88, 0, 102, 11}, 1, 0.05f)
        .addFrame(glm::vec4{88, 12, 102, 23}, 1, 0.05f)
        .addFrame(glm::vec4{103, 0, 117, 11}, 1, 0.05f)
        .addFrame(glm::vec4{103, 12, 117, 23}, 1, 0.05f)
        .setRepeat(true)
        ;
    gameStage_->app()->spriteAnimationHandler()->createAnimation(boosterEntity_, animationBuilder);
    auto& snd = registry_->emplace<ngn::Sound>(boosterEntity_, gameStage_->resources().boostSoundData);
    snd.setRepeat(true);
}

Player::~Player()
{
    registry_->destroy(boosterEntity_);
    registry_->destroy(entity_);
}

void Player::update(float deltaTime)
{
    laserReloadTimer_.update(deltaTime);

    handleInput(deltaTime);

    if (boosterActive_)
    {
        auto [pPos, pRot] = registry_->get<const ngn::Position, const ngn::Rotation>(entity_);
        auto [bPos, bRot] = registry_->get<ngn::Position, ngn::Rotation>(boosterEntity_);

        bPos.value = pPos.value - 28.0f * pRot.dir;
        bRot.angle = pRot.angle;
        bRot.update();
    }
}

void Player::kill()
{
    const auto playerPos = registry_->get<const ngn::Position>(entity_).value;
    gameStage_->explosions()->doExplosion(playerPos, Explosions::Type::One);

    registry_->remove<ngn::ActiveTag>(entity_);

    gameStage_->app()->spriteAnimationHandler()->stopAnimation(boosterEntity_);
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
    pos.value = gameStage_->board()->navigationGraph()->midPoint(startSector_);
    rot.angle = startOrientation_;
    rot.update();
    linVel.value = {};
    angVel.value = {};
    linFor.value = {};
    angFor.value = {};
    registry_->emplace_or_replace<ngn::TransformChangedTag>(entity_);
    registry_->emplace_or_replace<ngn::ActiveTag>(entity_);
    boosterActive_ = false;
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
        bool boosterActive = false;

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
            boosterActive = app->isKeyDown(GLFW_KEY_Q);
            const auto factor = boosterActive ? 11000.0f : 5000.0f;
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

        handleBoosterAction(boosterActive);
    }
}

void Player::handleBoosterAction(bool shouldBeActive)
{
    if (!boosterActive_ && shouldBeActive)
    {
        gameStage_->app()->spriteAnimationHandler()->startAnimation(boosterEntity_);
        const auto& snd = registry_->get<const ngn::Sound>(boosterEntity_);
        snd.play();
    }
    if (boosterActive_ && !shouldBeActive)
    {
        gameStage_->app()->spriteAnimationHandler()->stopAnimation(boosterEntity_);
        const auto& snd = registry_->get<const ngn::Sound>(boosterEntity_);
        snd.stop();
    }
    boosterActive_ = shouldBeActive;
}
