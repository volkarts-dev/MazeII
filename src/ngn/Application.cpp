// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Application.hpp"

#include "gfx/CommandBuffer.hpp"
#include "gfx/Pipeline.hpp"
#include "gfx/Renderer.hpp"
#include "Logging.hpp"
#include "Types.hpp"
#include <GLFW/glfw3.h>

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
    return gApplication;
}

Application::Application(ApplicationDelegate* delegate) :
    delegate_{delegate},
    window_{},
    renderer_{}
{
    assert(!gApplication);

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

    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetKeyCallback(window_, keyCallback);

    if (!delegate_->onInit(this))
        throw std::runtime_error("Failed to initialize app.");
}

Application::~Application()
{
    delegate_->onDone(this);

    delete renderer_;

    glfwDestroyWindow(window_);

    glfwTerminate();
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
        lastIterationTime = currentTime;

        update(deltaTime.count());
        draw(deltaTime.count());
    }

    renderer_->waitForDevice();
}

void Application::update(float deltaTime)
{
    delegate_->onUpdate(this, deltaTime);
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
