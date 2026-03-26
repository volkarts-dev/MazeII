// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Explosions.hpp"

#include "Application.hpp"
#include "GameStage.hpp"
#include "MazeComponents.hpp"
#include "MazeDelegate.hpp"
#include "audio/Sound.hpp"
#include "gfx/SpriteAnimator.hpp"
#include "phys/PhysComponents.hpp"

Explosions::Explosions(GameStage* gameStage) :
    gameStage_{gameStage},
    registry_{gameStage_->app()->registry()},
    world_{gameStage_->app()->world()}
{
}

Explosions::~Explosions()
{
    auto view = registry_->view<ExplosionTag>();
    registry_->destroy(view.begin(), view.end());
}

bool Explosions::allDone() const
{
    return gameStage_->app()->spriteAnimationHandler()->runningCount() == 0;
}

void Explosions::doExplosion(const glm::vec2& position, Type type)
{
    entt::entity entity{};

    const auto inactiveView = registry_->view<ExplosionTag>(entt::exclude<ngn::ActiveTag>);
    if (const auto it = inactiveView.begin(); it != inactiveView.end())
    {
        entity = *it;
    }
    else
    {
        entity = registry_->create();

        registry_->emplace<ngn::Position>(entity, position);

        registry_->emplace<ngn::Sprite>(entity, ngn::Sprite{
            .texCoords = {0, 0, 64, 64},
            .size{64, 64},
            .texture = 1,
        });

        ngn::SpriteAnimationBuilder animationBuilder{};
        animationBuilder
            .addFrame(glm::vec4{0, 137, 9, 146}, 1, 0.1f)
            .addFrame(glm::vec4{0, 147, 16, 162}, 1, 0.1f)
            .addFrame(glm::vec4{17, 137, 66, 183}, 1, 0.1f)
            .addFrame(glm::vec4{115, 137, 166, 193}, 1, 0.1f)
            .addFrame(glm::vec4{167, 137, 198, 165}, 1, 0.1f)
            .addFrame(glm::vec4{167, 166, 197, 195}, 1, 0.1f)
            ;
        gameStage_->app()->spriteAnimationHandler()->createAnimation(entity, animationBuilder);

        registry_->emplace<ngn::Sound>(entity);
        registry_->emplace<ExplosionTag>(entity);
    }

    auto [pos, rot, snd] = registry_->get<
            ngn::Position,
            ngn::Sprite,
            ngn::Sound>(entity);

    pos.value = position;

    switch (type)
    {
        case Type::One:
            snd.setBuffer(gameStage_->resources().explosionOneSoundData);
            break;
        case Type::Two:
            snd.setBuffer(gameStage_->resources().explosionTwoSoundData);
            break;
    }

    snd.play();

    registry_->emplace_or_replace<ngn::TransformChangedTag>(entity);

    gameStage_->app()->spriteAnimationHandler()->startAnimation(entity);
}

void Explosions::update(float deltaTime)
{
    NGN_UNUSED(deltaTime);
}
