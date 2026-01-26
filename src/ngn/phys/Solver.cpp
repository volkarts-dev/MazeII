// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Solver.hpp"

#include "CommonComponents.hpp"
#include "phys/PhysComponents.hpp"
#include <entt/entt.hpp>

namespace ngn {

void resolveCollisions(entt::registry* registry, const CollisionList& collisions)
{
    for (const auto& col : collisions)
    {
        resolveCollision(registry, col);
    }
}

void resolveCollision(entt::registry* registry, const Collision& collision)
{
    auto [bodyA, posA, velA] =
            registry->try_get<Body, Position, LinearVelocity>(collision.pair.bodyA);
    auto [bodyB, posB, velB] =
            registry->try_get<Body, Position, LinearVelocity>(collision.pair.bodyB);

    LinearVelocity nullVel{};

    if (!velA)
        velA = &nullVel;
    if (!velB)
        velB = &nullVel;

    const auto vd = velB->value - velA->value;

    float r = glm::dot(vd, collision.direction);
    if (r > 0.0f) // bodies are separating
        r = -r;

    float e = glm::max(1.0f, glm::max(bodyA->restitution, bodyB->restitution));

    float invMassSum = bodyA->invMass + bodyB->invMass;

    glm::vec2 impulse = (-e * r) * collision.direction;

    // apply impulse (TODO use force?)
    velA->value -= (bodyA->invMass / invMassSum) * impulse;
    velB->value += (bodyB->invMass / invMassSum)  * impulse;

    // position correction
    constexpr float percent = 0.2f;
    glm::vec2 correction = (collision.penetration / invMassSum) * percent * collision.direction;
    posA->value -= bodyA->invMass * correction;
    posB->value += bodyB->invMass * correction;
}

} // namespace ngn
