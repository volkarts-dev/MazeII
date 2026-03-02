// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "phys/Layers.hpp"

constexpr auto LayerBoundaries = ngn::Layers::B0;
constexpr auto LayerWalls = ngn::Layers::L0;

constexpr auto LayerNavHelpers = ngn::Layers::B1;
constexpr auto LayerNavEdges = ngn::Layers::L4;

constexpr auto LayerOpponents = ngn::Layers::B2;
constexpr auto LayerPlayer = ngn::Layers::L8;
constexpr auto LayerEnemies = ngn::Layers::L9;

constexpr auto LayerShots = ngn::Layers::B3;
constexpr auto LayerPlayerShots = ngn::Layers::L12;
constexpr auto LayerEnemyShots = ngn::Layers::L13;
