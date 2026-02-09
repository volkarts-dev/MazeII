// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "SteeringHelper.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace ngn {

namespace {

const auto Slice = glm::pi<float>() / 128.0f;
const auto SlowSlice = Slice * 2.0f;

inline float angleRange(float angle)
{
    auto norm = glm::mod(angle + glm::pi<float>(), glm::two_pi<float>());
    if (norm < 0.0f)
        norm += glm::two_pi<float>();
    return norm - glm::pi<float>();
}

} // namespace

float computeAngularForce(float currentAngle, float desiredAngle, float maxForce)
{
    const auto rotation = angleRange(desiredAngle - currentAngle);
    const auto rotationSize = glm::abs(rotation);

    if (rotationSize < Slice)
        return 0.0f;

    const auto offset = Slice * 0.9f;
    const auto accFactor = std::min((rotationSize - offset) / (SlowSlice - offset), 1.0f);

    return glm::sign(rotation) * maxForce * accFactor;
}

} // namespace ngn
