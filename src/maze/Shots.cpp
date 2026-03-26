// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Shots.hpp"

#include "Application.hpp"
#include "GameStage.hpp"
#include "Layers.hpp"
#include "MazeComponents.hpp"
#include "MazeDelegate.hpp"
#include "phys/PhysComponents.hpp"
#include <glm/gtx/norm.hpp>

Shots::Shots(GameStage* gameStage) :
    gameStage_{gameStage},
    registry_{gameStage_->app()->registry()},
    world_{gameStage_->app()->world()}
{
    collisionCallback_ = world_->addCollisionListener<&Shots::handleCollision>(this);
}

Shots::~Shots()
{
    auto view = registry_->view<ShotTag>();
    registry_->destroy(view.begin(), view.end());

    collisionCallback_.release();
}

void Shots::fireLaser(const glm::vec2& position, float rotation, bool player)
{
    entt::entity entity{};

    const auto inactiveView = registry_->view<ShotTag>(entt::exclude<ngn::ActiveTag>);
    if (const auto it = inactiveView.begin(); it != inactiveView.end())
    {
        entity = *it;
    }
    else
    {
        ActorCreateInfo createInfo{
            .sprite = {
                .size = {4, 12},
                .texture = 1,
            },
            .body = {
                .layers = player ? LayerPlayerShots : LayerEnemyShots,
                .invMass = 100000.0f,
                .restitution = 0.0f,
                .friction = 0.001f,
                .sensor = true,
                .useForce = false,
            },
            .shape = ngn::Shape{ngn::Circle{.center = {0, 2}, .radius = 2}},
            .active = false,
        };

        entity = gameStage_->createActor(createInfo);
        registry_->emplace<ShotSound>(entity);
        registry_->emplace<ShotInfo>(entity);
        registry_->emplace<ShotTag>(entity);

        auto& hitWallSound = registry_->emplace<HitWallSound>(entity);
        hitWallSound.setBuffer(gameStage_->resources().laserHitWallSoundData);
    }

    registry_->emplace<ngn::ActiveTag>(entity);

    auto [pos, rot, vel, spr, snd, info] = registry_->get<
            ngn::Position,
            ngn::Rotation,
            ngn::LinearVelocity,
            ngn::Sprite,
            ShotSound,
            ShotInfo>(entity);

    pos.value = position;

    rot.angle = rotation;
    rot.update();

    if (player)
    {
        const auto dyn = static_cast<float>(gameStage_->level() - 1) * 2.0f;
        vel.value = rot.dir * (400.0f + dyn);
    }
    else
    {
        const auto dyn = static_cast<float>(gameStage_->level() - 1) * 4.0f;
        vel.value = rot.dir * (80.0f + dyn);
    }

    spr.texCoords = player ? glm::vec4{84, 0, 87, 11} : glm::vec4{84, 12, 87, 23};

    snd.setBuffer(player ? gameStage_->resources().playerShotSoundData : gameStage_->resources().enemyShotSoundData);
    snd.play();

    info.sourceType = player ? ActorType::Player : ActorType::Enemy;

    registry_->emplace_or_replace<ngn::TransformChangedTag>(entity);
}

void Shots::update(float deltaTime)
{
    NGN_UNUSED(deltaTime);

    auto view = registry_->view<const ngn::Position, ShotTag, ngn::ActiveTag>();
    for (auto [e, pos] : view.each())
    {
        if (!gameStage_->testInSight(pos.value))
            registry_->remove<ngn::ActiveTag>(e);
    }
}

void Shots::reset()
{
    auto view = registry_->view<ShotTag, ngn::ActiveTag>();
    for (auto e : view)
    {
        registry_->remove<ngn::ActiveTag>(e);
    }
}

void Shots::handleCollision(const ngn::CollisionInfo& collision)
{
    const auto shotA = registry_->any_of<ShotTag>(collision.pair.bodyA);
    const auto shotB = registry_->any_of<ShotTag>(collision.pair.bodyB);

    auto handleHit = [this](entt::entity shot, entt::entity otherBody)
    {
        const auto otherSensor = registry_->get<const ngn::Body>(otherBody).sensor;
        if (otherSensor)
            return;

        const auto sourceType = registry_->get<const ShotInfo>(shot).sourceType;
        const auto isEnemy = registry_->any_of<EnemyTag>(otherBody);
        const auto isPlayer = registry_->any_of<PlayerTag>(otherBody);

        if ((sourceType == ActorType::Player && isPlayer) || (sourceType == ActorType::Enemy && isEnemy))
            return;

        registry_->remove<ngn::ActiveTag>(shot);

        if (isEnemy)
        {
            gameStage_->killEnemy(otherBody);
        }
        else if (isPlayer)
        {
            gameStage_->killPlayer();
        }
        else
        {
            const auto& snd = registry_->get<HitWallSound>(shot);
            snd.play();
        }
    };

    if (shotA && shotB)
        return;
    else if (shotA)
        handleHit(collision.pair.bodyA, collision.pair.bodyB);
    else if (shotB)
        handleHit(collision.pair.bodyB, collision.pair.bodyA);
}
