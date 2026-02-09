// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Types.hpp"
#include "ai/NavigationGraph.hpp"
#include "utils/StaticVector.hpp"

using NavIndex = ngn::NavigationGraph::IndexType;

template<typename T, std::size_t Size>
using NavArray = ngn::Array<T, Size, NavIndex>;

template<std::size_t Size>
using NavSectorArray = NavArray<NavIndex, Size>;

template<typename T, std::size_t Size>
using NavVector = ngn::StaticVector<T, Size, NavIndex>;

template<std::size_t Size>
using NavSectorVector = NavVector<NavIndex, Size>;

constexpr NavIndex NavSectorEdgeCount = ngn::NavigationGraph::Connections::Size;

enum class ActorType : uint32_t
{
    Player,
    Enemy,
    Shot,
};

class PlayerTag
{
};

class EnemyTag
{
};

class ShotTag
{
};

class ExplosionTag
{
};

class EnemyPathSectors
{
public:
    NavSectorArray<4> path{
        ngn::InvalidIndex<NavIndex>,
        ngn::InvalidIndex<NavIndex>,
        ngn::InvalidIndex<NavIndex>,
        ngn::InvalidIndex<NavIndex>,
    };
};

class EnemyPathPoints
{
public:
    NavArray<glm::vec2, 4> path{};
};

class ShotInfo
{
public:
    ActorType sourceType{};
};
