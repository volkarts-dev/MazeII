// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "NavigationGraph.hpp"

#include "Types.hpp"
#include <glm/gtx/norm.hpp>

namespace ngn {

NavigationGraph::NavigationGraph()
{
}

void NavigationGraph::addPoint(glm::vec2 point)
{
    points_.emplace_back(std::move(point));
}

NavigationGraph::Points NavigationGraph::points(IndexType index)
{
    const auto& sector = sectors_[index];
    return {points_[sector.p1], points_[sector.p2], points_[sector.p3], points_[sector.p4]};
}

void NavigationGraph::addSector(IndexType p1, IndexType p2, IndexType p3, IndexType p4)
{
    sectors_.emplace_back(p1, p2, p3, p4);
    midPoints_.emplace_back((points_[p1] + points_[p2] + points_[p3] + points_[p4]) * 0.25f);
}

void NavigationGraph::addConnections(IndexType c1, IndexType c2, IndexType c3, IndexType c4)
{
    connections_.push_back({c1, c2, c3, c4});
}

NavigationGraph::IndexType NavigationGraph::findNearerSector(IndexType start, const glm::vec2& point)
{
    IndexType nearestIndex = start;
    const auto baseLength2 = glm::length2(midPoints_[start] - point);
    for (IndexType i = 0; i < 4; i++)
    {
        const auto connection = connections_[start][i];
        if (connection == InvalidIndex<IndexType>)
            continue;
        const auto& midPoint = midPoints_[connection];
        const auto len2 = glm::length2(midPoint - point);
        if (len2 < baseLength2)
            nearestIndex = connection;
    }
    return nearestIndex;
}

} // namespace ngn
