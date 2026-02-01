// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

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

class ShotInfo
{
public:
    ActorType sourceType{};
};

class ExplosionTag
{
};
