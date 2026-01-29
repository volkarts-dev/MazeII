// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Level.hpp"

#include "Application.hpp"
#include "gfx/GFXComponents.hpp"
#include "phys/World.hpp"
#include <entt/entt.hpp>

namespace {

constexpr uint32_t MazeSize = 10;
constexpr uint32_t BlockSize = 128;

} // namespace

Level::Level(ngn::Application *app) :
    app_{app} ,
    registry_{app_->registry()}
{
    createMaze();
}

Level::~Level()
{
    registry_->destroy(walls_.begin(), walls_.end());
    registry_->destroy(sprites_.begin(), sprites_.end());
}

void Level::createMaze()
{
    auto* world = app_->world();

    ngn::BodyCreateInfo wallCreateInfo;
    wallCreateInfo.restitution = 1.5f;
    wallCreateInfo.invMass = 0;
    wallCreateInfo.dynamic = false;

    constexpr auto outerWallCount = (MazeSize * 2 + 1) * 4;
    constexpr auto innerWallCount = (MazeSize * MazeSize) * 4;
    const glm::vec2 offset{32, 32};

    walls_.resize(outerWallCount + innerWallCount);
    registry_->create(walls_.begin(), walls_.end());

    const float last = BlockSize * (MazeSize * 2 + 1);

    auto createWallBody = [reg = registry_, &wallCreateInfo, &offset, world]
            (entt::entity entity, const glm::vec2& start, const glm::vec2& end)
    {
        world->createBody(entity, wallCreateInfo, ngn::Shape{
            ngn::Line{.start = offset + start, .end = offset + end}
        });
        reg->emplace<ngn::ActiveTag>(entity);
    };

    // outer walls

    for (uint32_t i = 0; i < MazeSize * 2 + 1; i++)
    {
        const auto start = i * BlockSize;
        const auto end = start + BlockSize;

        createWallBody(walls_[i * 4 + 0], glm::vec2{start, 0}, glm::vec2{end, 0});

        createWallBody(walls_[i * 4 + 1], glm::vec2{start, last}, glm::vec2{end, last});

        createWallBody(walls_[i * 4 + 2], glm::vec2{0, start}, glm::vec2{0, end});

        createWallBody(walls_[i * 4 + 3], glm::vec2{last, start}, glm::vec2{last, end});
    }

    // inner walls

    for (uint32_t y = 0; y < MazeSize; y++)
    {
        for (uint32_t x = 0; x < MazeSize; x++)
        {
            const auto i = outerWallCount + (y * MazeSize + x) * 4;

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

    // sprites

    const glm::vec2 tileBase{0, 41};
    const glm::vec2 tileSize{32, 32};
    const glm::vec2 tileHalfSize = tileSize / 2.0f;
    const glm::vec2 tileOffset = offset + tileHalfSize;

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

        pos = tileOffset + glm::vec2{x1, last};
        coordsBase = tileBase + glm::vec2{tileSize.x, 0};
        createSprite(pos, coordsBase);

        pos = tileOffset + glm::vec2{-tileSize.x, y1};
        coordsBase = tileBase + glm::vec2{tileSize.x * 2, tileSize.y};
        createSprite(pos, coordsBase);

        pos = tileOffset + glm::vec2{last, y1};
        coordsBase = tileBase + glm::vec2{0, tileSize.y};
        createSprite(pos, coordsBase);
    }

    createSprite(tileOffset - tileSize, tileBase + glm::vec2{tileSize.x * 3, 0});
    createSprite(tileOffset + glm::vec2{last, -tileSize.y}, tileBase + glm::vec2{tileSize.x * 5, 0});
    createSprite(tileOffset + glm::vec2{-tileSize.x, last}, tileBase + glm::vec2{tileSize.x * 3, tileSize.y * 2});
    createSprite(tileOffset + glm::vec2{last, last}, tileBase + glm::vec2{tileSize.x * 5, tileSize.y * 2});

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
