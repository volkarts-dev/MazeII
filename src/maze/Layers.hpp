// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "phys/Layers.hpp"

constexpr auto LayerWalls = ngn::Layers::L1;
constexpr auto LayerBoundaries = ngn::Layers::B1;

constexpr auto LayerPlayer = ngn::Layers::L4;
constexpr auto LayerEnemies = ngn::Layers::L5;
constexpr auto LayerOpponents = ngn::Layers::B2;

constexpr auto LayerPlayerShots = ngn::Layers::L8;
constexpr auto LayerEnemyShots = ngn::Layers::L9;
constexpr auto LayerShots = ngn::Layers::B3;
