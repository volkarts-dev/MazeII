// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Enemies.hpp"

#include "Application.hpp"
#include "CommonComponents.hpp"
#include "GameStage.hpp"
#include "Layers.hpp"
#include "Level.hpp"
#include "Math.hpp"
#include "ai/SteeringHelper.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "phys/CollisionTests.hpp"
#include "phys/PhysComponents.hpp"
#include "phys/World.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

namespace {

constexpr float linearForce = 500.0f;
constexpr float UpdateTimeout = 0.0f;

class StartSector
{
public:
    NavIndex index;
};

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
    navigationGraph_{gameStage_->level()->navigationGraph()},
    updateTimer_{},
    randGenerator_{gameStage->app()->randomSeed()}
{
}

Enemies::~Enemies()
{
    auto view = registry_->view<EnemyTag>();
    registry_->destroy(view.begin(), view.end());
}

void Enemies::createEnemy(NavIndex startSector, float angle)
{
    const auto pos = navigationGraph_->midPoint(startSector);

    ActorCreateInfo createInfo{
        .position = pos,
        .rotation = angle,
        .sprite = {
            .texCoords = {39, 0, 84, 35},
            .size = {46, 36},
            .texture = 1,
        },
        .body = {
            .layers = LayerEnemies,
            .invMass = 1.f / 10.f,
            .restitution = 1.5f,
        },
        .shape = ngn::Shape{ngn::Circle{.center = {0, 2}, .radius = 17}},
    };
    const auto enemy = gameStage_->createActor(createInfo);

    registry_->emplace<EnemyTag>(enemy);
    auto& start = registry_->emplace<StartSector>(enemy);
    auto& info = registry_->emplace<EnemyInfo>(enemy);
    auto& sectors = registry_->emplace<EnemyPathSectors>(enemy);
    auto& points = registry_->emplace<EnemyPathPoints>(enemy);

    start.index = startSector;
    info.state = State::Wander;
    sectors.path[0] = startSector;
    points.path[0] = pos;
}

void Enemies::killEnemy(entt::entity enemy)
{
    registry_->remove<ngn::ActiveTag>(enemy);
    registry_->emplace<RespawnTimer>(enemy, 5.0f);
}

void Enemies::update(float deltaTime)
{
    updateTimer_.update(deltaTime);

    auto respawnView = registry_->view<RespawnTimer>();
    for (auto [e, timer] : respawnView.each())
    {
        timer.timeout -= deltaTime;
        if (timer.timeout <= 0.0f)
        {
            registry_->remove<RespawnTimer>(e);
            registry_->emplace<ngn::ActiveTag>(e);

            auto [start, pos, rot, sectors, points, info] = registry_->get<
                    const StartSector,
                    ngn::Position,
                    ngn::Rotation,
                    EnemyPathSectors,
                    EnemyPathPoints,
                    EnemyInfo>(e);

            pos.value = navigationGraph_->midPoint(start.index);
            rot.angle = glm::pi<float>();
            rot.update();

            info.state = State::Wander;
            sectors.path[0] = start.index;
            std::fill(sectors.path.begin() + 1, sectors.path.end(), ngn::InvalidIndex<NavIndex>);
            points.path[0] = pos.value;

            registry_->emplace_or_replace<ngn::TransformChangedTag>(e);
        }
    }

    bool doUpdateStep = updateTimer_.elapsed(UpdateTimeout).first;

    const auto targetView = registry_->view<
            const ngn::Position,
            const ngn::LinearVelocity,
            PlayerTag>();
    auto [tEnt, tPos, tVel] = *targetView.each().begin();

    auto view = registry_->view<
            const ngn::Position,
            const ngn::Rotation,
            const ngn::LinearVelocity,
            ngn::LinearForce,
            ngn::AngularForce,
            EnemyInfo,
            EnemyPathSectors,
            EnemyPathPoints,
            EnemyTag,
            ngn::ActiveTag>();
    for (auto [ent, pos, rot, linVel, linForce, angForce, info, sectors, points] : view.each())
    {
        if (doUpdateStep)
        {
            auto& currentSector = sectors.path[0];
            auto possibleNewSector = navigationGraph_->findNearerSector(currentSector, pos.value);
            if (possibleNewSector != currentSector) // Moved to other sector
            {
                if (possibleNewSector == sectors.path[1])
                {
                    for (NavIndex i = 1; i < sectors.path.size(); i++)
                    {
                        sectors.path[i - 1] = sectors.path[i];
                        points.path[i - 1] = points.path[i];
                    }
                    sectors.path[sectors.path.size() - 1] = ngn::InvalidIndex<NavIndex>;
                }
                else
                {
                    sectors.path[0] = possibleNewSector;
                    points.path[0] = navigationGraph_->midPoint(possibleNewSector);
                    std::fill(sectors.path.begin() + 1, sectors.path.end(), ngn::InvalidIndex<NavIndex>);
                }
                ngn::log::info("{}; {}-{}-{}", currentSector, sectors.path[0], sectors.path[1], sectors.path[2]);
            }
        }

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
                linForce.value = steeringSeek(pos.value, linVel.value, futureTPos);

                break;
            }

            case Evasion:
            {
                break;
            }

            case Wander:
            {
                bool filled = false;
                for (NavIndex i = 1; i < sectors.path.size(); i++)
                {
                    if (sectors.path[i] == ngn::InvalidIndex<NavIndex>)
                    {
                        const auto last = i > 1 ? sectors.path[i - 2] : ngn::InvalidIndex<NavIndex>;
                        const auto current = sectors.path[i - 1];
                        const auto next = findNextRandomSector(last, current);
                        sectors.path[i] = next;
                        points.path[i] = navigationGraph_->midPoint(next);
                        filled = true;
                    }
                }
                if (filled)
                {
                    ngn::log::info("New Path: {}-{}-{}", sectors.path[0], sectors.path[1], sectors.path[2]);
                }

                glm::vec2 headed{NAN, NAN};

                for (NavIndex i = 0; i < points.path.size() - 1; i++)
                {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
                    gameStage_->app()->debugRenderer()->drawLine(points.path[i], points.path[i + 1], ngn::Colors::Blue);
#endif

                    const auto ip = ngn::intersections(points.path[i], points.path[i + 1], pos.value, 64.0f);
                    if (!std::isnan(ip.first.x))
                    {
                        headed = ip.first;
#if !defined(NGN_ENABLE_VISUAL_DEBUGGING)
                        break;
#endif
                    }
                }

                if (!std::isnan(headed.x))
                {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
                    gameStage_->app()->debugRenderer()->drawCircle(headed, 3, ngn::Colors::Blue);
#endif

                    linForce.value += rot.dir * 500.0f;

                    const auto dir = headed - pos.value;
                    const auto maxAngForce = 20.0f;
                    angForce.value +=
                            ngn::computeAngularForce(rot.angle, ngn::atan2(dir.x, dir.y), maxAngForce);
                }

                break;
            }
        }
    }
}

NavIndex Enemies::findNextRandomSector(NavIndex last, NavIndex current)
{
    NavSectorVector<NavSectorEdgeCount> possibilities;
    const auto& connections = navigationGraph_->connections(current);
    for (NavIndex i = 0; i < connections.size(); i++)
    {
        if (connections[i] == ngn::InvalidIndex<NavIndex> || connections[i] == last)
            continue;
        possibilities.emplace_back(connections[i]);
    }

    std::uniform_int_distribution<NavIndex> distrib(0, possibilities.size() - 1);
    return possibilities[distrib(randGenerator_)];
}

bool Enemies::testInSight(entt::entity player, entt::entity enemy, const ngn::Line& lineOfSight)
{
    const auto lineAABB = ngn::calculateAABB(lineOfSight);
    bool blocking = false;
    world_->query(lineAABB, LayerPlayer, [&blocking, player, enemy](const ngn::TreeNode& node)
    {
        // TODO Do an actual collision check (not only an aabb test)
        blocking = node.entity != player && node.entity != enemy;
        return !blocking;
    });

    const auto diff2 = glm::length2(lineOfSight.end - lineOfSight.start);
    const bool inSight = !blocking && diff2 > 65536.0f && diff2 < 262144.0f;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    gameStage_->app()->debugRenderer()->drawArrow(
                lineOfSight.start, lineOfSight.end, 20.0f,
                (!blocking && diff2 < 262144.0f) ? ngn::Colors::Green : ngn::Colors::Red);
#endif

    return inSight;
}
