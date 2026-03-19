// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Timer.hpp"
#include "Types.hpp"
#include "audio/Sound.hpp"
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
    NavIndex last{ngn::InvalidIndex<NavIndex>};
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

class ShotSound : public ngn::Sound
{
};

class HitWallSound : public ngn::Sound
{
};

class StartSector
{
public:
    NavIndex index;
    float orientation;
};

class EvasionTimer : public ngn::Timer
{
};
