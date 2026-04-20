// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Enemies.hpp"

#include "Application.hpp"
#include "Board.hpp"
#include "CommonComponents.hpp"
#include "Explosions.hpp"
#include "GameStage.hpp"
#include "Layers.hpp"
#include "Math.hpp"
#include "MazeDelegate.hpp"
#include "Shots.hpp"
#include "ai/SteeringHelper.hpp"
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

constexpr float BodyRadius = 17.0f;
constexpr float SightLength = 512.0f;
constexpr float LinearForce = 500.0f;
constexpr float AngularForce = 20.0f;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
constexpr float UpdateTimeout = 0.0f;
#else
// TODO Support ai update at a given time interval and not on every frame
constexpr float UpdateTimeout = 0.0f; // 0.0625f;
#endif

inline void steeringSeek(ngn::LinearForce& linForce, ngn::AngularForce& angForce,
                         const ngn::Position& pos, const ngn::Rotation& rot,
                         const glm::vec2& target, float maxLinForce, float maxAngForce)
{
    linForce.value += rot.dir * maxLinForce;

    const auto dir = target - pos.value;
    angForce.value += ngn::computeAngularForce(rot.angle, ngn::math::atan2(dir.x, dir.y), maxAngForce);
}

} // namespace

Enemies::Enemies(GameStage* gameStage) :
    gameStage_{gameStage},
    registry_{gameStage_->app()->registry()},
    world_{gameStage_->app()->world()},
    navigationGraph_{gameStage_->board()->navigationGraph()},
    updateTimer_{},
    randGenerator_{gameStage->app()->randomSeed()}
{
}

Enemies::~Enemies()
{
    auto view = registry_->view<EnemyTag>();
    registry_->destroy(view.begin(), view.end());
}

void Enemies::createEnemy(NavIndex startSector, float startOrientation)
{
    const auto textureId = gameStage_->delegate()->resources().spriteTexture;

    const auto pos = navigationGraph_->midPoint(startSector);

    ActorCreateInfo createInfo{
        .position = pos,
        .rotation = startOrientation,
        .sprite = {
            .texCoords = {39, 0, 84, 35},
            .size = {46, 36},
            .texture = textureId,
        },
        .body = {
            .layers = LayerEnemies,
            .invMass = 1.f / 10.f,
            .restitution = 1.5f,
        },
        .shape = ngn::Shape{ngn::Circle{.center = {0, 2}, .radius = BodyRadius}},
    };
    const auto enemy = gameStage_->createActor(createInfo);

    registry_->emplace<EnemyTag>(enemy);
    registry_->emplace<EvasionTimer>(enemy);
    auto& start = registry_->emplace<StartSector>(enemy);
    auto& info = registry_->emplace<EnemyInfo>(enemy);
    auto& sectors = registry_->emplace<EnemyPathSectors>(enemy);
    auto& points = registry_->emplace<EnemyPathPoints>(enemy);

    start.index = startSector;
    start.orientation = startOrientation;
    info.state = State::Wander;
    sectors.path[0] = startSector;
    points.path[0] = pos;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    registry_->emplace<EnemyDebugState>(enemy);
#endif
}

void Enemies::killEnemy(entt::entity enemy)
{
    const auto& pos = registry_->get<const ngn::Position>(enemy);
    gameStage_->explosions()->doExplosion(pos.value, Explosions::Type::Two);

    registry_->remove<ngn::ActiveTag>(enemy);
    auto view = registry_->view<EnemyTag, ngn::ActiveTag>();
    if (view.begin() == view.end())
    {
        allEnemiesDownSignal_.publish();
    }
}

void Enemies::reset()
{
    auto view = registry_->view<
            const StartSector,
            ngn::Position,
            ngn::Rotation,
            ngn::LinearVelocity,
            ngn::AngularVelocity,
            ngn::LinearForce,
            ngn::AngularForce,
            EnemyPathSectors,
            EnemyPathPoints,
            EnemyInfo,
            EnemyTag
            >();
    for (auto [e, start, pos, rot, linVel, angVel, linFor, angFor, sectors, points, info] : view.each())
    {
        if (!registry_->all_of<ngn::ActiveTag>(e))
            registry_->emplace<ngn::ActiveTag>(e);

        pos.value = navigationGraph_->midPoint(start.index);
        rot.angle = start.orientation;
        rot.update();
        linVel.value = {};
        angVel.value = {};
        linFor.value = {};
        angFor.value = {};

        info.state = State::Wander;
        sectors.path[0] = start.index;
        std::fill(sectors.path.begin() + 1, sectors.path.end(), ngn::InvalidIndex<NavIndex>);
        points.path[0] = pos.value;

        registry_->emplace_or_replace<ngn::TransformChangedTag>(e);
    }
}

void Enemies::update(float deltaTime)
{
    updateTimer_.update(deltaTime);

    bool doUpdateStep = updateTimer_.isElapsed(UpdateTimeout).first;

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
            EvasionTimer,
            EnemyTag,
            ngn::ActiveTag>();
    for (auto [ent, pos, rot, linVel, linForce, angForce, info, sectors, points, et] : view.each())
    {
        if (doUpdateStep)
        {
            et.update(deltaTime);

            trackVisitedSectors(sectors, points, pos.value);
        }

        // TODO This should also be guarded by doUpdateStep, but that would break the debug display
        const auto lineToTarget = tPos.value - pos.value;
        const auto lineToTargetLen = glm::length(lineToTarget);
        const auto sightDir = lineToTarget / lineToTargetLen;
        const auto sightDirTarget = pos.value + sightDir * glm::min(SightLength, lineToTargetLen);
        const auto inRange = glm::length2(sightDirTarget - pos.value) < (SightLength * SightLength - 1.0f);
        const auto targetInSight = inRange && testInSight(pos.value, sightDirTarget);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        auto& debugState = registry_->get<EnemyDebugState>(ent);
        if (gameStage_->debugShowAIStates())
        {
            gameStage_->app()->debugRenderer()->drawArrow(
                        pos.value, sightDirTarget, 20.0f,
                        targetInSight ? ngn::Colors::Green : ngn::Colors::Red);
        }
#endif

        const auto searchWayStart = pos.value + rot.dir * BodyRadius;
        const auto searchWayEnd = pos.value + rot.dir * BodyRadius * 5.0f;
        const auto obstacle = findObstacle(ent, searchWayStart, searchWayEnd, BodyRadius);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        debugState.searchWayStart = searchWayStart;
        debugState.searchWayEnd = searchWayEnd;
        debugState.obstacle = obstacle;
#endif

        // TODO Take relative velocity into account
        if (obstacle.found)
        {
            linForce.value += -obstacle.dir * obstacle.depth * 20.0f;
        }

        switch (info.state)
        {
            using enum State;

            case Persuit:
            {
                if (doUpdateStep)
                {
                    if (!targetInSight)
                    {
                        info.state = State::Wander;
                        break;
                    }
                    else if (testOrientation(pos.value, rot.angle, sightDirTarget))
                    {
                        const auto start = pos.value + rot.dir * 20.0f;
                        gameStage_->shots()->fireLaser(start, rot.angle, false);
                        info.state = State::Evasion;
                        break;
                    }
                }

                steeringSeek(linForce, angForce, pos, rot, tPos.value, LinearForce, AngularForce);

                break;
            }

            case Evasion:
            {
                et.restart();

                sectors.path[1] = sectors.last;
                points.path[1] = navigationGraph_->midPoint(sectors.last);
                std::fill(sectors.path.begin() + 2, sectors.path.end(), ngn::InvalidIndex<NavIndex>);

                info.state = State::Wander;

                break;
            }

            case Wander:
            {
                if (et.elapsedTime() > 5.0f && targetInSight)
                {
                    std::uniform_real_distribution<float> distrib(0.0f, 1.0f);
                    const auto prop = distrib(randGenerator_);

                    if (prop < (0.1f + static_cast<float>(gameStage_->level() - 1) * 0.05f))
                    {
                        info.state = State::Persuit;
                    }
                    else
                    {
                        et.restart();
                    }

                    break;
                }

                for (NavIndex i = 1; i < sectors.path.size(); i++)
                {
                    if (sectors.path[i] == ngn::InvalidIndex<NavIndex>)
                    {
                        const auto last = i > 1 ? sectors.path[i - 2] : sectors.last;
                        const auto current = sectors.path[i - 1];
                        const auto next = findNextRandomSector(last, current);
                        sectors.path[i] = next;
                        points.path[i] = navigationGraph_->midPoint(next);
                    }
                }

                auto headed = points.path[0];

                for (NavIndex i = 0; i < points.path.size() - 1; i++)
                {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
                    if (gameStage_->debugShowAIStates())
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
                    if (gameStage_->debugShowAIStates())
                        gameStage_->app()->debugRenderer()->drawCircle(headed, 3, ngn::Colors::Blue);
#endif

                    steeringSeek(linForce, angForce, pos, rot, headed, LinearForce, AngularForce);
                }

                break;
            }
        }
    }
}

inline void Enemies::trackVisitedSectors(EnemyPathSectors& sectors, EnemyPathPoints& points, const glm::vec2& pos)
{
    auto& currentSector = sectors.path[0];
    auto possibleNewSector = navigationGraph_->findNearerSector(currentSector, pos);
    if (possibleNewSector != currentSector) // Moved to other sector
    {
        sectors.last = sectors.path[0];

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
    }
}

inline NavIndex Enemies::findNextRandomSector(NavIndex last, NavIndex current)
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

bool Enemies::testInSight(const glm::vec2& origin, const glm::vec2& target)
{
    const ngn::Line sightLine = {.start = origin, .end = target};
    const auto lineAABB = ngn::calculateAABB(sightLine);
    bool blocking = false;
    world_->query(lineAABB, LayerBoundaries, [this, &sightLine, &blocking](const ngn::TreeNode& node)
    {
        const auto shape = registry_->get<ngn::Shape>(node.entity);
        ngn::Collision collision;
        blocking = ngn::testCollision(collision, sightLine, shape);
        return !blocking;
    });
    return !blocking;
}

bool Enemies::testOrientation(const glm::vec2& origin, float dir, const glm::vec2& target)
{
    const auto ot = target - origin;
    const auto dirToTarget = ngn::math::atan2(ot.x, ot.y);
    return ngn::math::angleDiff(dir, dirToTarget) < ngn::math::TwoPI * 0.017f;
}

Enemies::FindObstacleResult Enemies::findObstacle(entt::entity self, const glm::vec2& origin,
                                                  const glm::vec2& target, float halfWidth)
{
    // TODO move findObstacle() to engine/ai

    // This function assumes, that origin is in front of the enemy

    const ngn::Capsule way = {.start = origin, .end = target, .radius = halfWidth};
    const auto lineAABB = ngn::calculateAABB(way);

    FindObstacleResult result{};
    float closestDist2 = std::numeric_limits<float>::max();
    ngn::Collision closestCollision{};
    entt::entity closestEntity{};

    constexpr auto ObstaclesLayers = LayerNavHelpers | LayerOpponents | LayerShots;

    world_->query(lineAABB, ObstaclesLayers, [&](const ngn::TreeNode& node)
    {
        if (node.entity == self)
            return true;

        const auto shape = registry_->get<ngn::Shape>(node.entity);
        ngn::Collision collision;
        if (!ngn::testCollision(collision, way, shape))
            return true;

        const auto dist2 = glm::length2(collision.point - origin);
        if (dist2 < closestDist2)
        {
            result.found = true;
            closestDist2 = dist2;
            closestCollision = collision;
            closestEntity = node.entity;
        }

        return true;
    });

    if (result.found)
    {
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
        registry_->get<EnemyDebugState>(self).closestCollision = closestCollision;
#endif
        const auto dynamic = registry_->all_of<ngn::DynamicTag>(closestEntity);

        const auto ot = target - origin;
        const auto op = closestCollision.point - origin;
        const auto t = glm::dot(ot, op) / glm::length2(ot);
        const auto p = origin + ot * t;
        glm::vec2 pp{};
        if (dynamic)
        {
            // always evade to the same direction when the obstacle is also able to avoid
            pp = glm::vec2{ot.y, -ot.x};
        }
        else
        {
            // otherwise evade away from obstacle
            pp = closestCollision.point - p;
            if (glm::length2(pp) < 25.0f)
                pp = glm::vec2{ot.y, -ot.x};
        }
        result.dir = glm::normalize(pp);
        result.depth = closestCollision.penetration;
    }
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    else
    {
        registry_->get<EnemyDebugState>(self).closestCollision = {};
    }
#endif

    return result;
}

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)

void Enemies::debugDraw()
{
    if (gameStage_->debugShowAIStates())
    {
        auto view = registry_->view<const EnemyDebugState, ngn::ActiveTag>();
        for (auto [e, state] : view.each())
        {
            gameStage_->app()->debugRenderer()->drawCapsule(
                        state.searchWayStart, state.searchWayEnd, BodyRadius,
                        ngn::Colors::Yellow);

            if (state.closestCollision.colliding)
                gameStage_->app()->debugRenderer()->drawCircle(state.closestCollision.point, 3, ngn::Colors::Yellow);

            if (state.obstacle.found)
                gameStage_->app()->debugRenderer()->drawArrow(
                    state.searchWayStart,
                    state.searchWayStart - state.obstacle.dir * state.obstacle.depth * 2.0f,
                    10.0f,
                    ngn::Colors::Green);
        }
    }
}

#endif
