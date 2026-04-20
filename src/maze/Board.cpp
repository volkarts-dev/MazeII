// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Board.hpp"

#include "Layers.hpp"
#include "MazeComponents.hpp"
#include "Application.hpp"
#include "gfx/GfxComponents.hpp"
#include "phys/World.hpp"
#include <entt/entt.hpp>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

namespace {

/*

BIG:
  0|  1|  2|  3|  4|  5|  6|  7|  8|  9| 10| 11| 12| 13| 14| 15| 16| 17| 18| 19| 20
 21|   | 22|   | 23|   | 24|   | 25|   | 26|   | 27|   | 28|   | 29|   | 30|   | 31
 32| 33| 34| 35| 36| 37| 38| 39| 40| 41| 42| 43| 44| 45| 46| 47| 48| 49| 50| 51| 52
 53|   | 54|   | 55|   | 56|   | 57|   | 58|   | 59|   | 60|   | 61|   | 62|   | 63
 64| 65| 66| 67| 68| 69| 70| 71| 72| 73| 74| 75| 76| 77| 78| 79| 80| 81| 82| 83| 84
 85|   | 86|   | 87|   | 88|   | 89|   | 90|   | 91|   | 92|   | 93|   | 94|   | 95
 96| 97| 98| 99|100|101|102|103|104|105|106|107|108|109|110|111|112|113|114|115|116
117|   |118|   |119|   |120|   |121|   |122|   |123|   |124|   |125|   |126|   |127
128|129|130|131|132|133|134|135|136|137|138|139|140|141|142|143|144|145|146|147|148
149|   |150|   |151|   |152|   |153|   |154|   |155|   |156|   |157|   |158|   |159
160|161|162|163|164|165|166|167|168|169|170|171|172|173|174|175|176|177|178|179|180
181|   |182|   |183|   |184|   |185|   |186|   |187|   |188|   |189|   |190|   |191
192|193|194|195|196|197|198|199|200|201|202|203|204|205|206|207|208|209|210|211|212
213|   |214|   |215|   |216|   |217|   |218|   |219|   |220|   |221|   |222|   |223
224|225|226|227|228|229|230|231|232|233|234|235|236|237|238|239|240|241|242|243|244
245|   |246|   |247|   |248|   |249|   |250|   |251|   |252|   |253|   |254|   |255
256|257|258|259|260|261|262|263|264|265|266|267|268|269|270|271|272|273|274|275|276
277|   |278|   |279|   |280|   |281|   |282|   |283|   |284|   |285|   |286|   |287
288|289|290|291|292|293|294|295|296|297|298|299|300|301|302|303|304|305|306|307|308
309|   |310|   |311|   |312|   |313|   |314|   |315|   |316|   |317|   |318|   |319
320|321|322|323|324|325|326|327|328|329|330|331|332|333|334|335|336|337|338|339|340

SMALL:
  0|  1|  2|  3|  4|  5|  6|  7|  8|  9| 10| 11| 12|
 13|   | 14|   | 15|   | 16|   | 17|   | 18|   | 19|
 20| 21| 22| 23| 24| 25| 26| 27| 28| 29| 30| 31| 32|
 33|   | 34|   | 35|   | 36|   | 37|   | 38|   | 39|
 40| 41| 42| 43| 44| 45| 46| 47| 48| 49| 50| 51| 52|
 53|   | 54|   | 55|   | 56|   | 57|   | 58|   | 59|
 60| 61| 62| 63| 64| 65| 66| 67| 68| 69| 70| 71| 72|
 73|   | 74|   | 75|   | 76|   | 77|   | 78|   | 79|
 80| 81| 82| 83| 84| 85| 86| 87| 88| 89| 90| 91| 92|
 93|   | 94|   | 95|   | 96|   | 97|   | 98|   | 99|
100|101|102|103|104|105|106|107|108|109|110|111|112|
113|   |114|   |115|   |116|   |117|   |118|   |119|
120|121|122|123|124|125|126|127|128|129|130|131|132|

*/

constexpr uint32_t MazeSize = 6;
constexpr uint32_t BlockSize = 128;
constexpr auto OuterWallCount = (MazeSize * 2 + 1) * 4;
constexpr auto InnerWallCount = (MazeSize * MazeSize) * 4;
constexpr float Last = BlockSize * (MazeSize * 2 + 1);
const glm::vec2 Offset{32, 32};

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
const glm::vec4 ColorLightYellow = {1.0f, 1.0f, 0.5f, 0.2f};
#endif

} // namespace

Board::Board(ngn::Application *app) :
    app_{app} ,
    registry_{app_->registry()},
    navigationGraph_{new ngn::NavigationGraph{}}
{
    createWalls();
    createSprites();
    createNavigationGraph();
}

Board::~Board()
{
    registry_->destroy(walls_.begin(), walls_.end());
    registry_->destroy(sprites_.begin(), sprites_.end());

    delete navigationGraph_;
}

glm::vec2 Board::dimension() const
{
    return Offset * 2.0f + glm::vec2{1} * static_cast<float>(BlockSize * (MazeSize * 2 + 1));
}

void Board::createWalls()
{
    auto* world = app_->world();

    ngn::BodyCreateInfo wallCreateInfo{
        .layers = LayerWalls,
        .invMass = 0,
        .restitution = 1.5f,
        .dynamic = false,
    };

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

void Board::createSprites()
{
    const glm::vec2 tileBase{0, 41};
    const glm::vec2 tileSize{32, 32};
    const glm::vec2 tileHalfSize = tileSize / 2.0f;
    const glm::vec2 tileOffset = Offset + tileHalfSize;

    auto createSprite = [this, &tileSize](const glm::vec2& pos, const glm::vec2& coordsBase)
    {
        const auto e = registry_->create();
        registry_->emplace<ngn::Position>(e, pos);
        registry_->emplace<ngn::Sprite>(
                    e, ngn::Sprite{.texCoords{coordsBase, coordsBase + tileSize}, .size = tileSize, .texture = 1});
        registry_->emplace<ngn::ActiveTag>(e);
        registry_->emplace<ngn::StaticTag>(e);
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

void Board::createNavigationGraph()
{
    constexpr auto SectorsPerRow = MazeSize * 2 + 1;
    constexpr auto PointsPerRow = SectorsPerRow + 1;

    auto* world = app_->world();

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

    ngn::BodyCreateInfo edgeCreateInfo{
        .layers = LayerNavEdges,
        .invMass = 0,
        .sensor = true,
        .dynamic = false,
    };

    for (uint32_t y = 1; y < MazeSize * 2; y++)
    {
        for (uint32_t x = 1; x < MazeSize * 2; x++)
        {
            const auto entity = registry_->create();
            const auto pos = glm::vec2{
                x * BlockSize,
                y * BlockSize,
            } + Offset;
            world->createBody(entity, edgeCreateInfo, ngn::Shape{
                ngn::Circle{.center = pos, .radius = 10}
            });
            registry_->emplace<ngn::ActiveTag>(entity);
        }
    }
}

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)

void Board::debugDrawState(ngn::DebugRenderer* debugRenderer)
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
