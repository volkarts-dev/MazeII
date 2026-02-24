// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "utils/Array.hpp"

namespace ngn {

class Line;

class NavigationGraph
{
public:
    using IndexType = uint16_t;

    using Points = Array<glm::vec2, 4, IndexType>;

    using Connections = Array<IndexType, 4, IndexType>;

    class Sector
    {
    public:
        IndexType p1;
        IndexType p2;
        IndexType p3;
        IndexType p4;
    };

public:
    NavigationGraph();

    void addPoint(glm::vec2 point);
    void addSector(IndexType p1, IndexType p2, IndexType p3, IndexType p4);
    void addConnections(IndexType p1, IndexType p2, IndexType p3, IndexType p4);

    IndexType sectorCount() const { return static_cast<IndexType>(sectors_.size()); }

    Points points(IndexType index);
    const glm::vec2& midPoint(IndexType index) { return midPoints_[index]; }
    const Connections& connections(IndexType index) const { return connections_[index]; }

    IndexType findNearerSector(IndexType start, const glm::vec2& point);

private:

    std::vector<glm::vec2> points_;
    std::vector<glm::vec2> midPoints_;
    std::vector<Sector> sectors_;
    std::vector<Connections> connections_;
};

} // namespace ngn
