// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "CommonComponents.hpp"
#include "Solver.hpp"
#include "phys/PhysComponents.hpp"
#include <entt/entt.hpp>

namespace ngn {

void Solver::resolveCollisions(entt::registry* registry, const CollisionList& collisions)
{
    for (const auto& col : collisions)
    {
        resolveCollision(registry, col);
    }
}

void Solver::resolveCollision(entt::registry* registry, const Collision& collision)
{
    const auto [bodyA, posA, velocityA] =
            registry->get<Body, Position, LinearVelocity>(collision.pair.bodyA);
    const auto [bodyB, posB, velocityB] =
            registry->get<Body, Position, LinearVelocity>(collision.pair.bodyB);

    const auto vd = velocityB.value - velocityA.value;

    float r = glm::dot(vd, collision.direction);
    if (r > 0.0f) // bodies are separating
        r = -r;

    float e = glm::max(1.0f, glm::max(bodyA.restitution, bodyB.restitution));

    float invMassSum = bodyA.invMass + bodyB.invMass;

    glm::vec2 impulse = (-e * r) * collision.direction;

    // apply impulse (TODO use force?)
    velocityA.value -= (bodyA.invMass / invMassSum) * impulse;
    velocityB.value += (bodyB.invMass / invMassSum)  * impulse;

    // position correction
    constexpr float percent = 0.2f;
    glm::vec2 correction = (collision.penetration / invMassSum) * percent * collision.direction;
    posA.value -= bodyA.invMass * correction;
    posB.value += bodyB.invMass * correction;
}

} // namespace ngn
