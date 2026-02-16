// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Level.hpp"

#include "Layers.hpp"
#include "MazeComponents.hpp"
#include "Application.hpp"
#include "gfx/GFXComponents.hpp"
#include "phys/World.hpp"
#include <entt/entt.hpp>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

namespace {

constexpr uint32_t MazeSize = 10;
constexpr uint32_t BlockSize = 128;
constexpr auto OuterWallCount = (MazeSize * 2 + 1) * 4;
constexpr auto InnerWallCount = (MazeSize * MazeSize) * 4;
constexpr float Last = BlockSize * (MazeSize * 2 + 1);
const glm::vec2 Offset{32, 32};

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
const glm::vec4 ColorLightYellow = {1.0f, 1.0f, 0.5f, 0.2f};
#endif

} // namespace

Level::Level(ngn::Application *app) :
    app_{app} ,
    registry_{app_->registry()},
    navigationGraph_{new ngn::NavigationGraph{}}
{
    createWalls();
    createSprites();
    createNavigationGraph();
}

Level::~Level()
{
    registry_->destroy(walls_.begin(), walls_.end());
    registry_->destroy(sprites_.begin(), sprites_.end());

    delete navigationGraph_;
}

void Level::createWalls()
{
    auto* world = app_->world();

    ngn::BodyCreateInfo wallCreateInfo;
    wallCreateInfo.restitution = 1.5f;
    wallCreateInfo.invMass = 0;
    wallCreateInfo.dynamic = false;
    wallCreateInfo.layers = LayerWalls;

    walls_.resize(OuterWallCount + InnerWallCount);
    registry_->create(walls_.begin(), walls_.end());

    auto createWallBody = [reg = registry_, &wallCreateInfo, world]
            (entt::entity entity, const glm::vec2& start, const glm::vec2& end)
    {
        world->createBody(entity, wallCreateInfo, ngn::Shape{
            ngn::Line{.start = Offset + start, .end = Offset + end}
        });
        reg->emplace<ngn::ActiveTag>(entity);
    };

    // outer walls

    for (uint32_t i = 0; i < MazeSize * 2 + 1; i++)
    {
        const auto start = i * BlockSize;
        const auto end = start + BlockSize;

        createWallBody(walls_[i * 4 + 0], glm::vec2{start, 0}, glm::vec2{end, 0});

        createWallBody(walls_[i * 4 + 1], glm::vec2{start, Last}, glm::vec2{end, Last});

        createWallBody(walls_[i * 4 + 2], glm::vec2{0, start}, glm::vec2{0, end});

        createWallBody(walls_[i * 4 + 3], glm::vec2{Last, start}, glm::vec2{Last, end});
    }

    // inner walls

    for (uint32_t y = 0; y < MazeSize; y++)
    {
        for (uint32_t x = 0; x < MazeSize; x++)
        {
            const auto i = OuterWallCount + (y * MazeSize + x) * 4;

            const auto x1 = BlockSize + x * 2 * BlockSize;
            const auto x2 = x1 + BlockSize;
            const auto y1 = BlockSize + y * 2 * BlockSize;
            const auto y2 = y1 + BlockSize;

            createWallBody(walls_[i + 0], glm::vec2{x1, y1}, glm::vec2{x2, y1});

            createWallBody(walls_[i + 1], glm::vec2{x2, y1}, glm::vec2{x2, y2});

            createWallBody(walls_[i + 2], glm::vec2{x2, y2}, glm::vec2{x1, y2});

            createWallBody(walls_[i + 3], glm::vec2{x1, y2}, glm::vec2{x1, y1});
        }
    }
}

void Level::createSprites()
{
    const glm::vec2 tileBase{0, 41};
    const glm::vec2 tileSize{32, 32};
    const glm::vec2 tileHalfSize = tileSize / 2.0f;
    const glm::vec2 tileOffset = Offset + tileHalfSize;

    auto createSprite = [this, &tileSize](const glm::vec2& pos, const glm::vec2& coordsBase)
    {
        auto e = registry_->create();
        registry_->emplace<ngn::Position>(e, pos);
        registry_->emplace<ngn::Sprite>(
                    e, ngn::Sprite{.texCoords{coordsBase, coordsBase + tileSize}, .size = tileSize, .texture = 1});
        registry_->emplace<ngn::ActiveTag>(e);
    };

    // outer sprites

    for (uint32_t i = 0; i < (MazeSize * 2 + 1) * 4; i++)
    {
        const auto x1 = static_cast<float>(i) * tileSize.x;
        const auto y1 = static_cast<float>(i) * tileSize.y;

        glm::vec2 pos = tileOffset + glm::vec2{x1, -tileSize.y};
        glm::vec2 coordsBase = tileBase + glm::vec2{tileSize.x, tileSize.y * 2};
        createSprite(pos, coordsBase);

        pos = tileOffset + glm::vec2{x1, Last};
        coordsBase = tileBase + glm::vec2{tileSize.x, 0};
        createSprite(pos, coordsBase);

        pos = tileOffset + glm::vec2{-tileSize.x, y1};
        coordsBase = tileBase + glm::vec2{tileSize.x * 2, tileSize.y};
        createSprite(pos, coordsBase);

        pos = tileOffset + glm::vec2{Last, y1};
        coordsBase = tileBase + glm::vec2{0, tileSize.y};
        createSprite(pos, coordsBase);
    }

    createSprite(tileOffset - tileSize, tileBase + glm::vec2{tileSize.x * 3, 0});
    createSprite(tileOffset + glm::vec2{Last, -tileSize.y}, tileBase + glm::vec2{tileSize.x * 5, 0});
    createSprite(tileOffset + glm::vec2{-tileSize.x, Last}, tileBase + glm::vec2{tileSize.x * 3, tileSize.y * 2});
    createSprite(tileOffset + glm::vec2{Last, Last}, tileBase + glm::vec2{tileSize.x * 5, tileSize.y * 2});

    // inner sprites

    for (uint32_t y = 0; y < MazeSize; y++)
    {
        for (uint32_t x = 0; x < MazeSize; x++)
        {
            const auto x1 = BlockSize + x * 2 * BlockSize;
            const auto y1 = BlockSize + y * 2 * BlockSize;

            glm::vec2 blockPos = tileOffset + glm::vec2{x1, y1};

            glm::vec2 pos = blockPos + glm::vec2{0, 0};
            glm::vec2 coordsBase{tileBase};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x, 0};
            coordsBase = tileBase + glm::vec2{tileSize.x, 0};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x * 2, 0};
            coordsBase = tileBase + glm::vec2{tileSize.x, 0};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x * 3, 0};
            coordsBase = tileBase + glm::vec2{tileSize.x * 2, 0};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{0, tileSize.y};
            coordsBase = tileBase + glm::vec2{0, tileSize.y};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x * 3, tileSize.y};
            coordsBase = tileBase + glm::vec2{tileSize.x * 2, tileSize.y};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{0, tileSize.y * 2};
            coordsBase = tileBase + glm::vec2{0, tileSize.y};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x * 3, tileSize.y * 2};
            coordsBase = tileBase + glm::vec2{tileSize.x * 2, tileSize.y};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{0, tileSize.y * 3};
            coordsBase = tileBase + glm::vec2{0, tileSize.y * 2};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x, tileSize.y * 3};
            coordsBase = tileBase + glm::vec2{tileSize.x, tileSize.y * 2};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x * 2, tileSize.y * 3};
            coordsBase = tileBase + glm::vec2{tileSize.x, tileSize.y * 2};
            createSprite(pos, coordsBase);

            pos = blockPos + glm::vec2{tileSize.x * 3, tileSize.y * 3};
            coordsBase = tileBase + glm::vec2{tileSize.x * 2, tileSize.y * 2};
            createSprite(pos, coordsBase);
        }
    }
}

void Level::createNavigationGraph()
{
    constexpr auto SectorsPerRow = MazeSize * 2 + 1;
    constexpr auto PointsPerRow = SectorsPerRow + 1;

    for (uint32_t y = 0; y < PointsPerRow; y++)
    {
        for (uint32_t x = 0; x < PointsPerRow; x++)
        {
            navigationGraph_->addPoint(Offset + glm::vec2{x * BlockSize, y * BlockSize});
        }
    }

    auto yBase = [](uint32_t y)
    {
        constexpr uint32_t HalfSectorsPerRow = SectorsPerRow / 2 + 1;
        return std::make_pair(
            ((y + 1) / 2) * SectorsPerRow + (y / 2) * HalfSectorsPerRow,
            y & 1 ? 2u : 1u
        );
    };

    auto top = [yBase](uint32_t x, uint32_t y)
    {
        if ((y == 0) || (x & 1))
            return ngn::InvalidIndex16;
        const auto base = yBase(y - 1);
        return static_cast<uint16_t>(base.first + x / base.second);
    };

    auto right = [yBase](uint32_t x, uint32_t y)
    {
        if ((x == (SectorsPerRow - 1)) || (y & 1))
            return ngn::InvalidIndex16;
        const auto base = yBase(y);
        return static_cast<uint16_t>(base.first + x / base.second + 1);
    };

    auto bottom = [yBase](uint32_t x, uint32_t y)
    {
        if ((y == (SectorsPerRow - 1)) || (x & 1))
            return ngn::InvalidIndex16;
        const auto base = yBase(y + 1);
        return static_cast<uint16_t>(base.first + x / base.second);
    };

    auto left = [yBase](uint32_t x, uint32_t y)
    {
        if ((x == 0) || (y & 1))
            return ngn::InvalidIndex16;
        const auto base = yBase(y);
        return static_cast<uint16_t>(base.first + x / base.second - 1);
    };

    for (uint32_t y = 0; y < SectorsPerRow; y++)
    {
        for (uint32_t x = 0; x < SectorsPerRow; x += 1 + (y & 1))
        {
            const auto base = static_cast<uint16_t>(y * PointsPerRow + x);

            navigationGraph_->addSector(base, base + 1, base + PointsPerRow + 1, base + PointsPerRow + 0);

            navigationGraph_->addConnections(top(x, y), right(x, y), bottom(x, y), left(x, y));
        }
    }
}

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)

void Level::debugDrawState(ngn::DebugRenderer* debugRenderer)
{
    for (NavIndex i = 0; i < navigationGraph_->sectorCount(); i++)
    {
        const auto points = navigationGraph_->points(i);

        const auto diag1 = (points[2] - points[0]) * 0.9f;
        const auto diag2 = (points[3] - points[1]) * 0.9f;

        glm::vec2 p[] = {
            points[2] - diag1,
            points[3] - diag2,
            points[0] + diag1,
            points[1] + diag2,
        };

        const auto& midPoint = navigationGraph_->midPoint(i);
        const auto& connections = navigationGraph_->connections(i);

        debugRenderer->drawCircle(midPoint, 3, ColorLightYellow);

        for (NavIndex e = 0; e < connections.size(); e++)
        {
            if (connections[e] == ngn::InvalidIndex<NavIndex>)
                continue;

            const auto mp = (p[e] + p[static_cast<NavIndex>((e + 1) % 4)]) / 2.0f;

            debugRenderer->drawLine(midPoint, mp, ColorLightYellow);
        }
    }
}

#endif
