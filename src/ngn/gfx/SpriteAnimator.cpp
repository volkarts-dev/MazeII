// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "SpriteAnimator.hpp"

#include "CommonComponents.hpp"
#include "SpriteAnimation.hpp"
#include "gfx/GFXComponents.hpp"
#include <entt/entt.hpp>

namespace ngn {

namespace {

class SpriteAnimationInfo
{
public:
    uint32_t framesStart{};
    uint32_t framesEnd{};
    bool repeat{};
    bool playing{};
};

} // namespace

SpriteAnimationBuilder& SpriteAnimationBuilder::addFrame(glm::vec4 coords, uint32_t texture, float time)
{
    frames_.emplace_back(std::move(coords), texture, time);
    return *this;
}

SpriteAnimationBuilder& SpriteAnimationBuilder::setRepeat(bool repeat)
{
    repeat_ = repeat;
    return *this;
}

SpriteAnimationBuilder& SpriteAnimationBuilder::setStart(bool start)
{
    start_ = start;
    return *this;
}

// *********************************************************************************************************************

SpriteAnimator::SpriteAnimator(entt::registry* registry) :
    registry_{registry}
{
}

void SpriteAnimator::createAnimation(entt::entity entity, const SpriteAnimationBuilder& builder)
{
    auto& info = registry_->emplace<SpriteAnimationInfo>(entity);

    info.framesStart = static_cast<uint32_t>(frames_.size());
    info.framesEnd = info.framesStart + static_cast<uint32_t>(builder.frames_.size());
    info.playing = builder.start_;
    info.repeat = builder.repeat_;

    std::copy(builder.frames_.begin(), builder.frames_.end(), std::back_inserter(frames_));

    if (info.playing)
        startAnimation(entity);
}

void SpriteAnimator::startAnimation(entt::entity entity)
{
    auto& info = registry_->get<SpriteAnimationInfo>(entity);
    registry_->emplace<SpriteAnimation>(entity, 0, frames_[info.framesStart].time);
    updateSprite(entity, info.framesStart);
}

void SpriteAnimator::stopAnimation(entt::entity entity)
{
    registry_->remove<SpriteAnimation>(entity);
}

void SpriteAnimator::update(float deltaTime)
{
    auto view = registry_->view<SpriteAnimation, ActiveTag>();
    for (auto [e, anim] : view.each())
    {
        anim.timeout -= deltaTime;
        if (anim.timeout <= 0.0f)
        {
            const auto& info = registry_->get<SpriteAnimationInfo>(e);

            anim.frame++;

            if (anim.frame >= info.framesEnd)
            {
                if (info.repeat)
                {
                    anim.frame = 0;
                    anim.timeout = frames_[anim.frame].time;
                    updateSprite(e, anim.frame);
                }
                else
                {
                    stopAnimation(e);
                }
            }
            else
            {
                anim.timeout = frames_[anim.frame].time;
                updateSprite(e, anim.frame);
            }
        }
    }
}

void SpriteAnimator::updateSprite(entt::entity entity, uint32_t frame)
{
    auto& sprite = registry_->get<Sprite>(entity);
    sprite.texCoords = frames_[frame].texCoords;
    sprite.texture = frames_[frame].texture;
}

} // namespace ngn
