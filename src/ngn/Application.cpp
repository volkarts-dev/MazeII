// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Application.hpp"

#include "Timer.hpp"
#include "gfx/CommandBuffer.hpp"
#include "gfx/FontMaker.hpp"
#include "gfx/FontRenderer.hpp"
#include "gfx/Pipeline.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "gfx/Renderer.hpp"
#include "phys/World.hpp"
#include "Allocators.hpp"
#include "Logging.hpp"
#include "Types.hpp"
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>
#include <cassert>
#include <cstdlib>

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
#include "gfx/DebugRenderer.hpp"
#endif

namespace ngn {

namespace {

Application* gApplication{};

void errorCallback(int error, const char* description)
{
    log::error("GLFW error: {} ({})", description, error);
}

} // namespace

ApplicationStage::~ApplicationStage() = default;

ApplicationDelegate::~ApplicationDelegate() = default;

Application* Application::get()
{
    assert(gApplication);
    return gApplication;
}

Application::Application(ApplicationDelegate* delegate) :
    delegate_{delegate},
    window_{},
    renderer_{},
    frameMemoryArena_{},
    spriteRenderer_{},
    fontRenderer_{},
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    debugRenderer_{},
#endif
    world_{},
    stage_{},
    nextStage_{},
    exitCode_{0}
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

    frameMemoryArena_ = new MemoryArena{delegate_->requiredFrameMemeory()};

    registry_ = new entt::registry{};

    world_ = new World(registry_);

    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetKeyCallback(window_, keyCallback);

    stage_ = delegate_->onInit(this);
    if (!stage_)
        throw std::runtime_error("Failed to initialize app.");

    stage_->onActivate(this);
}

Application::~Application()
{
    stage_->onDeactivate(this);

    delegate_->onDone(this);

    delete world_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    if (debugRenderer_)
        delete debugRenderer_;
#endif

    if (fontRenderer_)
        delete fontRenderer_;
    if (spriteRenderer_)
        delete spriteRenderer_;

    delete frameMemoryArena_;

    delete renderer_;

    glfwDestroyWindow(window_);

    glfwTerminate();

    gApplication = nullptr;
}

void Application::createRenderers(const RendererCreateInfo& createInfo)
{
    if (createInfo.spriteRenderer)
    {
        spriteRenderer_ = new SpriteRenderer{registry_, renderer_, createInfo.spriteBatchCount};

        if (createInfo.fontRenderer)
        {
            if (!createInfo.fontMaker)
                throw std::runtime_error("The FontRenderer requires a FontMaker");

            fontRenderer_ = new ngn::FontRenderer{spriteRenderer_, createInfo.fontMaker->compile()};
        }
    }
    else if (createInfo.fontRenderer)
    {
        throw std::runtime_error("The FontRenderer requires a SpriteRenderer");
    }

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    if (createInfo.debugRenderer)
    {
        debugRenderer_ = new ngn::DebugRenderer{renderer_, createInfo.debugBatchCount};
    }
#endif
}

void Application::activateStage(ApplicationStage* stage)
{
    nextStage_ = stage;
}

void Application::quit(int exitCode)
{
    exitCode_ = exitCode;

    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

entt::entity Application::createActor(Position pos, Rotation rot, Scale sca)
{
    const auto entity = registry_->create();

    registry_->emplace<Position>(entity, std::move(pos));
    auto& rotation = registry_->emplace<Rotation>(entity, std::move(rot));
    registry_->emplace<Scale>(entity, std::move(sca));
    registry_->emplace<TransformChanged>(entity, true);

    rotation.update();

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

int Application::exec()
{
    Timer fpsTimer;
    Timer fpsStatTimer;
    Timer memStatTimer;
    double frameCount{};

    while (!glfwWindowShouldClose(window_))
    {
        frameMemoryArena_->reset();

        if (nextStage_)
        {
            stage_->onDeactivate(this);
            stage_ = nextStage_;
            nextStage_ = nullptr;
            stage_->onActivate(this);
        }

        glfwPollEvents();

        const auto tick = fpsTimer.elapsed();
        const auto deltaTime = Duration<float>{tick.second}.count();

        if (const auto fpsStat = fpsStatTimer.elapsed(Duration<double>{5.0}); fpsStat.first)
        {
            const auto fps = frameCount / fpsStat.second.count();
            frameCount = 0;

            ngn::log::info("FPS: {:.1f}", fps);
        }
        frameCount += 1.0;

        update(deltaTime);

        draw(deltaTime);

        if (const auto memStat = memStatTimer.elapsed(Duration<double>{5.0}); memStat.first)
        {
            ngn::log::info("F-MEM: {}/{}, alloc: {} ({}), dealloc: {} ({})",
                           Bytes{frameMemoryArena_->allocated()}, Bytes{frameMemoryArena_->capacity()},
                           Bytes{frameMemoryArena_->statAllocatedSize()}, frameMemoryArena_->statAllocatedCount(),
                           Bytes{frameMemoryArena_->statDeallocatedSize()}, frameMemoryArena_->statDeallocatedCount());
        }
    }

    renderer_->waitForDevice();

    return exitCode_;
}

void Application::update(float deltaTime)
{
    stage_->onUpdate(this, deltaTime);

    world_->update(deltaTime);
}

void Application::draw(float deltaTime)
{
    stage_->onDraw(this, deltaTime);
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

    stage_->onKeyEvent(this, GLFW_PRESS, key);
}

void Application::handleKeyRelease(int key, int scancode, int mods)
{
    NGN_UNUSED(scancode);
    NGN_UNUSED(mods);

    stage_->onKeyEvent(this, GLFW_RELEASE, key);
}

} // namespace ngn
