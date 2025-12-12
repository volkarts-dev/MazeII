// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Application.hpp"

#include "gfx/CommandBuffer.hpp"
#include "gfx/Pipeline.hpp"
#include "gfx/Renderer.hpp"
#include "phys/World.hpp"
#include "Allocators.hpp"
#include "Logging.hpp"
#include "Types.hpp"
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>
#include <cassert>
#include <cstdlib>

namespace ngn {

namespace {

Application* gApplication{};

void errorCallback(int error, const char* description)
{
    log::error("GLFW error: {} ({})", description, error);
}

} // namespace

Application* Application::get()
{
    assert(gApplication);
    return gApplication;
}

Application::Application(ApplicationDelegate* delegate) :
    delegate_{delegate},
    window_{},
    renderer_{},
    frameAllocator_{},
    world_{}
{
    assert(!gApplication);
    gApplication = this;

    log::set_level(log::level::trace);

    if (!glfwInit())
        throw std::runtime_error("Failed to init glfw");

    glfwSetErrorCallback(errorCallback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(800, 600, "MazeII", nullptr, nullptr);
    if (!window_)
        throw std::runtime_error("Failed to create window");

    glfwSetWindowUserPointer(window_, this);

    renderer_ = new Renderer{window_};

    frameAllocator_ = new LinearAllocator{delegate_->requiredFrameMemeory()};

    registry_ = new entt::registry{};

    world_ = new World(registry_);

    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetKeyCallback(window_, keyCallback);

    if (!delegate_->onInit(this))
        throw std::runtime_error("Failed to initialize app.");
}

Application::~Application()
{
    delete world_;

    delegate_->onDone(this);

    delete frameAllocator_;

    delete renderer_;

    glfwDestroyWindow(window_);

    glfwTerminate();

    gApplication = nullptr;
}

entt::entity Application::createActor(Position pos, Rotation rot, Scale sca)
{
    const auto entity = registry_->create();

    registry_->emplace<Position>(entity, std::move(pos));
    registry_->emplace<Rotation>(entity, std::move(rot));
    registry_->emplace<Scale>(entity, std::move(sca));

    return entity;
}

bool Application::isKeyDown(int key) const
{
    return glfwGetKey(window_, key) == GLFW_PRESS;
}

bool Application::isKeyUp(int key) const
{
    return glfwGetKey(window_, key) == GLFW_RELEASE;
}

void Application::exec()
{
    Clock clock;

    auto lastIterationTime = clock.now();

    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();

        const auto currentTime = clock.now();
        const Duration<float> deltaTime = currentTime - lastIterationTime;
        const float dt = deltaTime.count();
        lastIterationTime = currentTime;

        update(dt);

        draw(dt);
    }

    renderer_->waitForDevice();
}

void Application::update(float deltaTime)
{
    delegate_->onUpdate(this, deltaTime);

    world_->update(deltaTime);
}

void Application::draw(float deltaTime)
{
    delegate_->onDraw(this, deltaTime);
}

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    NGN_UNUSED(width);
    NGN_UNUSED(height);

    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    app->renderer_->triggerFramebufferResized();
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
        app->handleKeyPress(key, scancode, mods);
    else if (action == GLFW_RELEASE)
        app->handleKeyRelease(key, scancode, mods);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void Application::handleKeyPress(int key, int scancode, int mods)
{
    NGN_UNUSED(scancode);
    NGN_UNUSED(mods);

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void Application::handleKeyRelease(int key, int scancode, int mods)
{
    NGN_UNUSED(key);
    NGN_UNUSED(scancode);
    NGN_UNUSED(mods);
}

} // namespace ngn
