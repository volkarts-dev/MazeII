// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Solver.hpp"

#include "CommonComponents.hpp"
#include "phys/PhysComponents.hpp"
#include <entt/entt.hpp>

namespace ngn {

void resolveCollisions(entt::registry* registry, const CollisionInfoList& collisions)
{
    for (const auto& col : collisions)
    {
        resolveCollision(registry, col);
    }
}

void resolveCollision(entt::registry* registry, const CollisionInfo& collision)
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

    float r = glm::dot(vd, collision.coll.direction);
    if (r > 0.0f) // bodies are separating
        r = -r;

    float e = glm::max(bodyA->restitution, bodyB->restitution);

    float invMassSum = bodyA->invMass + bodyB->invMass;

    glm::vec2 impulse = (-e * r) * collision.coll.direction;

    // apply impulse (TODO use force?)
    velA->value -= (bodyA->invMass / invMassSum) * impulse;
    velB->value += (bodyB->invMass / invMassSum)  * impulse;

    // position correction
    constexpr float percent = 0.2f;
    glm::vec2 correction = (collision.coll.penetration / invMassSum) * percent * collision.coll.direction;
    posA->value -= bodyA->invMass * correction;
    posB->value += bodyB->invMass * correction;

    registry->emplace_or_replace<TransformChangedTag>(collision.pair.bodyA);
    registry->emplace_or_replace<TransformChangedTag>(collision.pair.bodyB);
}

} // namespace ngn
