// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <glm/glm.hpp>

namespace ngn {

class Body
{
public:
    float invMass;
    float friction;
    float restitution;
    bool sensor;
    bool fastMoving;
};

class LinearForce
{
public:
    glm::vec2 value{};
};

class AngularForce
{
public:
    float value{};
};

class LinearVelocity
{
public:
    glm::vec2 value{};
};

class AngularVelocity
{
public:
    float value{};
};

class TransformChangedTag
{
};

} // namespace
