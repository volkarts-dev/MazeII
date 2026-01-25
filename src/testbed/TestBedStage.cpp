// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "TestBedStage.hpp"

#include "Math.hpp"
#include "phys/Functions.hpp"
#include "phys/PhysComponents.hpp"
#include "phys/World.hpp"
#include "gfx/FontMaker.hpp"
#include "gfx/UiRenderer.hpp"
#include "gfx/GFXComponents.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "CommonComponents.hpp"
#include "TestBedAssets.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <entt/entt.hpp>
#include <GLFW/glfw3.h>
#include <glm/gtc/constants.hpp>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

TestBedStage::TestBedStage(ngn::Application* app) :
    app_{app}
{
    app_->spriteRenderer()->addImages({{testbed::assets::player_png()}});

    ngn::FontMaker fontMaker{app_->renderer(), 256};
    fontMaker.addFont(testbed::assets::liberation_mono_ttf(), 20);
    app_->uiRenderer()->setFontCollection(fontMaker.compile());










    // TEMP

    auto* registry = app_->registry();

    walls_.reserve(6);

    auto createWall = [this](const glm::vec2& start, const glm::vec2& end)
    {
        walls_.push_back(app_->createActor(start));

        ngn::BodyCreateInfo createInfo;
        createInfo.restitution = 1.5f;
        createInfo.invMass = 0;
        createInfo.dynamic = false;
        app_->world()->createBody(walls_.back(), createInfo, ngn::Shape{
                                     ngn::Line{.start = {0, 0}, .end = end - start}
                                 });
    };

    createWall({10, 10}, {400, 10});
    createWall({400, 10}, {790, 10});
    createWall({790, 10}, {790, 590});
    createWall({10, 10}, {10, 590});
    createWall({10, 590}, {400, 500});
    createWall({400, 500}, {790, 590});



    ngn::BodyCreateInfo createInfo;

    player_ = app_->createActor({400, 300});

    registry->emplace<ngn::Sprite>(player_, ngn::Sprite{
                                        .texCoords = {0, 0, 64, 64},
                                        .size{64, 64},
                                        .texture = 1,
                                    });

    createInfo.restitution = 1.5f;
    createInfo.invMass = 1.f / 10.f;
    app_->world()->createBody(player_, createInfo, ngn::Shape{ngn::Circle{.radius = 32}});


    enemy_ = app_->createActor({600, 300});

    registry->emplace<ngn::Sprite>(enemy_, ngn::Sprite{
                                        .texCoords = {0, 0, 64, 64},
                                        .size{64, 64},
                                        .texture = 1,
                                    });

    createInfo.restitution = 1.5f;
    createInfo.invMass = 1.f / 1.f;
    app_->world()->createBody(enemy_, createInfo, ngn::Shape{ngn::Circle{.radius = 32}});


    obstacle_ = app_->createActor({300, 300});

    createInfo.restitution = 1.5f;
    createInfo.invMass = 0;
    app_->world()->createBody(obstacle_, createInfo, ngn::Shape{ngn::Capsule{.start = {0, -70}, .end = {0, 70}, .radius = 32}});

    // /TEMP
}

TestBedStage::~TestBedStage()
{
}

void TestBedStage::onActivate()
{
}

void TestBedStage::onDeactivate()
{
}

void TestBedStage::onKeyEvent(ngn::InputAction action, int key, ngn::InputMods mods)
{
    NGN_UNUSED(mods);

    if (action == ngn::InputAction::Press && key == GLFW_KEY_ESCAPE)
        app_->quit();
}

void TestBedStage::onUpdate(float deltaTime)
{
    app_->spriteRenderer()->updateView(
                glm::lookAt(
                    glm::vec3(400.0f, 300.0f, 0.5f),
                    glm::vec3(400.0f, 300.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                ));

    app_->uiRenderer()->updateView(
                glm::lookAt(
                    glm::vec3(400.0f, 300.0f, 0.5f),
                    glm::vec3(400.0f, 300.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                    ));

    // ****************************************************

    if (app_->isKeyDown(GLFW_KEY_LEFT))
    {
        auto& force = app_->registry()->get<ngn::AngularForce>(player_).value;
        force += 20.f;
    }
    if (app_->isKeyDown(GLFW_KEY_RIGHT))
    {
        auto& force = app_->registry()->get<ngn::AngularForce>(player_).value;
        force -= 20.f;
    }
    if (app_->isKeyDown(GLFW_KEY_UP))
    {
        auto [force, rot] = app_->registry()->get<ngn::LinearForce, ngn::Rotation>(player_);
        force.value -= rot.dir * 1200.f;
    }
    if (app_->isKeyDown(GLFW_KEY_DOWN))
    {
        auto [force, rot] = app_->registry()->get<ngn::LinearForce, ngn::Rotation>(player_);
        force.value += rot.dir * 1200.f;
    }

    // ****************************************************
    auto [obstacleRot, obstacleTc] = app_->registry()->get<ngn::Rotation, ngn::TransformChanged>(obstacle_);

    obstacleRot.angle +=  ngn::PI / 20.0f * deltaTime;
    obstacleTc.value = true;

    // ****************************************************

    app_->spriteRenderer()->renderSpriteComponents(app_->registry());

    // ****************************************************

    app_->uiRenderer()->writeText(0, "Hello Maze ][", 400, 50);

    // ****************************************************

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    app_->debugRenderer()->updateView(
                glm::lookAt(
                    glm::vec3(400.0f, 300.0f, 0.5f),
                    glm::vec3(400.0f, 300.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                ));

    app_->world()->debugDrawState(app_->debugRenderer(), true, true, true, true);
#endif
}
