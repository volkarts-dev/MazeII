// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <entt/fwd.hpp>

namespace ngn {

class SpriteAnimationFrame
{
public:
    glm::vec4 texCoords;
    uint32_t texture;
    float time;
};

// *********************************************************************************************************************

class SpriteAnimationBuilder
{
public:
    SpriteAnimationBuilder& addFrame(glm::vec4 coords, uint32_t texture, float time);
    SpriteAnimationBuilder& setRepeat(bool repeat);
    SpriteAnimationBuilder& setStart(bool start);

    std::vector<SpriteAnimationFrame> frames_{};
    bool repeat_{false};
    bool start_{false};
};

// *********************************************************************************************************************

class SpriteAnimator
{
public:
    SpriteAnimator(entt::registry* registry);

    void createAnimation(entt::entity entity, const SpriteAnimationBuilder& builder);
    void startAnimation(entt::entity entity);
    void stopAnimation(entt::entity entity);

    void update(float deltaTime);

private:
    void updateSprite(entt::entity entity, uint32_t frame);

private:
    entt::registry* registry_;
    std::vector<SpriteAnimationFrame> frames_;
};

} // namespace ngn
