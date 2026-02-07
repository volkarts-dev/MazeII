// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Enemies.hpp"

#include "phys/PhysComponents.hpp"
#include "phys/World.hpp"
#include "Application.hpp"
#include "CommonComponents.hpp"
#include "GameStage.hpp"
#include "MazeComponents.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

namespace {

constexpr float linearForce = 500.0f;
constexpr float UpdateTimeout = 0.0f;

class RespawnTimer
{
public:
    float timeout;
};

glm::vec2 steeringSeek(const glm::vec2& pos, const glm::vec2& vel, const glm::vec2& target)
{
    const auto desiredVel = glm::normalize(target - pos) * linearForce;
    return desiredVel - vel;
}

} // namespace

Enemies::Enemies(GameStage* gameStage) :
    gameStage_{gameStage},
    registry_{gameStage_->app()->registry()},
    world_{gameStage_->app()->world()},
    updateTimer_{}
{
}

Enemies::~Enemies()
{
    auto view = registry_->view<EnemyTag>();
    registry_->destroy(view.begin(), view.end());
}

void Enemies::createEnemy(glm::vec2 pos, float angle)
{
    ActorCreateInfo createInfo{
        .position = pos,
        .rotation = angle,
        .sprite = {
            .texCoords = {39, 0, 84, 35},
            .size = {46, 36},
            .texture = 1,
        },
        .body = {
            .invMass = 1.f / 10.f,
            .restitution = 1.5f,
        },
        .shape = ngn::Shape{ngn::Circle{.center = {0, 2}, .radius = 17}},
    };
    const auto enemy = gameStage_->createActor(createInfo);

    registry_->emplace<EnemyTag>(enemy);
    registry_->emplace<EnemyInfo>(enemy);
}

void Enemies::killEnemy(entt::entity enemy)
{
    registry_->remove<ngn::ActiveTag>(enemy);
    registry_->emplace<RespawnTimer>(enemy, 5.0f);
}

void Enemies::update(float deltaTime)
{
    auto respawnView = registry_->view<RespawnTimer>();
    for (auto [e, timer] : respawnView.each())
    {
        timer.timeout -= deltaTime;
        if (timer.timeout <= 0.0f)
        {
            registry_->remove<RespawnTimer>(e);
            registry_->emplace<ngn::ActiveTag>(e);

            auto [pos, rot] = registry_->get<ngn::Position, ngn::Rotation>(e);
            pos.value = {352, 352};
            rot.angle = 0.0f;
            rot.update();
            registry_->emplace_or_replace<ngn::TransformChangedTag>(e);
        }
    }

    bool doUpdateStep{}; // some things must not be done every frame
    updateTimer_ += deltaTime;
    if (updateTimer_ > UpdateTimeout)
    {
        updateTimer_ = 0.0f;
        doUpdateStep = true;
    }

    const auto targetView = registry_->view<
            const PlayerTag,
            const ngn::Position,
            const ngn::LinearVelocity>();
    auto [tEnt, tPos, tVel] = *targetView.each().begin();

    auto view = registry_->view<
            const EnemyTag,
            const ngn::ActiveTag,
            const ngn::Position,
            const ngn::LinearVelocity,
            ngn::LinearForce,
            EnemyInfo>();
    for (auto [ent, pos, vel, force, info] : view.each())
    {
        const ngn::Line lineOfSight{pos.value, tPos.value};

        switch (info.state)
        {
            using enum State;

            case Idle:
            {
                if (doUpdateStep)
                {
                    if (testInSight(tEnt, ent, lineOfSight))
                    {
                        info.state = State::Persuit;
                    }
                }
                break;
            }

            case Persuit:
            {
                if (doUpdateStep)
                {
                    if (!testInSight(tEnt, ent, lineOfSight))
                    {
                        info.state = State::Idle;
                    }
                }

                const auto futureTPos = tPos.value + tVel.value;
                force.value = steeringSeek(pos.value, vel.value, futureTPos);
                break;
            }

            case Evasion:
            {
                break;
            }
        }
    }
}

bool Enemies::testInSight(entt::entity player, entt::entity enemy, const ngn::Line& lineOfSight)
{
    const auto lineAABB = ngn::calculateAABB(lineOfSight);
    bool blocking = false;
    world_->query(lineAABB, [&blocking, player, enemy](auto e, auto)
    {
        blocking = e != player && e != enemy;
        return !blocking;
    });

    const auto diff2 = glm::length2(lineOfSight.end - lineOfSight.start);
    const bool inSight = !blocking && diff2 > 65536.0f && diff2 < 262144.0f;

//#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
//    gameStage_->app()->debugRenderer()->drawArrow(lineOfSight.start, lineOfSight.end, 20.0f, inSight ? ngn::Colors::Green : ngn::Colors::Red);
//#endif

    return inSight;
}
