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

    const auto config = delegate->applicationConfig(this);

    window_ = glfwCreateWindow(config.windowWidth, config.windowHeight, config.windowTitle, nullptr, nullptr);
    if (!window_)
        throw std::runtime_error("Failed to create window");

    glfwSetWindowUserPointer(window_, this);

    renderer_ = new Renderer{window_};


    frameMemoryArena_ = new MemoryArena{config.requiredMemory};

    registry_ = new entt::registry{};

    world_ = new World{this};

    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetKeyCallback(window_, keyCallback);

    if (config.spriteRenderer)
    {
        spriteRenderer_ = new SpriteRenderer{registry_, renderer_, config.spriteBatchCount};

        if (config.fontRenderer)
        {
            fontRenderer_ = new ngn::FontRenderer{spriteRenderer_};
        }
    }
    else if (config.fontRenderer)
    {
        throw std::runtime_error("The FontRenderer requires a SpriteRenderer");
    }

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    if (config.debugRenderer)
    {
        debugRenderer_ = new ngn::DebugRenderer{renderer_, config.debugBatchCount};
    }
#endif

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
    NGN_UNUSED(deltaTime);

    const auto imageIndex = renderer_->startFrame();
    if (imageIndex == ngn::InvalidIndex)
        return;

    auto* commandBuffer = renderer_->currentCommandBuffer();

    commandBuffer->begin(imageIndex);

    if (spriteRenderer_)
        spriteRenderer_->draw(commandBuffer);

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    if (debugRenderer_)
        debugRenderer_->draw(commandBuffer);
#endif
    commandBuffer->end();

    renderer_->submit(commandBuffer);

    renderer_->endFrame(imageIndex);
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
    NGN_UNUSED(scancode);

    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    app->stage_->onKeyEvent(app, toInputAction(action), key, toInputMods(mods));
}

} // namespace ngn
