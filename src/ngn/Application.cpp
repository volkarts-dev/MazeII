// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Application.hpp"

#include "Instrumentation.hpp"
#include "Timer.hpp"
#include "audio/Audio.hpp"
#include "gfx/CommandBuffer.hpp"
#include "gfx/FontMaker.hpp"
#include "gfx/UiRenderer.hpp"
#include "gfx/Pipeline.hpp"
#include "gfx/SpriteRenderer.hpp"
#include "gfx/Renderer.hpp"
#include "phys/World.hpp"
#include "Allocators.hpp"
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
    uiRenderer_{},
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    debugRenderer_{},
#endif
    audio_{},
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
        spriteRenderer_ = new SpriteRenderer{renderer_, config.spriteBatchCount};
    }

    if (config.fontRenderer)
    {
        uiRenderer_ = new ngn::UiRenderer{renderer_, config.fontBatchCount};
    }

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    if (config.debugRenderer)
    {
        debugRenderer_ = new ngn::DebugRenderer{renderer_, config.debugBatchCount};
    }
#endif

    if (config.audio)
        audio_ = new Audio{};

    stage_ = delegate_->onInit(this);
    if (!stage_)
        throw std::runtime_error("Failed to initialize app.");

    stage_->onActivate();
    stage_->onWindowResize(windowSize());
}

Application::~Application()
{
    stage_->onDeactivate();

    delegate_->onDone(this);

    delete world_;

    delete audio_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    delete debugRenderer_;
#endif

    delete uiRenderer_;

    delete spriteRenderer_;

    delete frameMemoryArena_;

    delete renderer_;

    glfwDestroyWindow(window_);

    glfwTerminate();

    gApplication = nullptr;
}

glm::vec2 Application::windowSize() const
{
    int width{};
    int height{};
    glfwGetFramebufferSize(window_, &width, &height);
    return glm::vec2{width, height};
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

entt::entity Application::createActor(glm::vec2 pos, float rot, glm::vec2 sca, bool active)
{
    const auto entity = registry_->create();

    registry_->emplace<Position>(entity, pos);

    auto& rotation = registry_->emplace<Rotation>(entity, glm::vec2{1, 0}, rot);
    rotation.update();

    registry_->emplace<Scale>(entity, sca);

    if (active)
        registry_->emplace<ActiveTag>(entity);

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
    Timer statTimer;
    double frameCount{};

    NGN_INSTRUMENTATION_MAIN_START();

    while (!glfwWindowShouldClose(window_))
    {
        frameMemoryArena_->reset();

        if (nextStage_)
        {
            stage_->onDeactivate();
            stage_ = nextStage_;
            nextStage_ = nullptr;
            stage_->onActivate();
            stage_->onWindowResize(windowSize());
        }

        glfwPollEvents();

        const auto tick = fpsTimer.elapsed(true);
        const auto deltaTime = Duration<float>{tick.second}.count();

        update(deltaTime);

        draw(deltaTime);

#if defined(NGN_ENABLE_INSTRUMENTATION)
        if (const auto stat = statTimer.elapsed(); frameCount >= 5000.0)
#else
        if (const auto stat = statTimer.elapsed(Duration<double>{5.0}); stat.first)
#endif
        {
            ngn::log::info("FPS: {:.1f}, F-MEM: {}/{}, alloc: {} ({}), dealloc: {} ({})",
                           frameCount / stat.second.count(),
                           Bytes{frameMemoryArena_->allocated()}, Bytes{frameMemoryArena_->capacity()},
                           Bytes{frameMemoryArena_->statAllocatedSize()}, frameMemoryArena_->statAllocatedCount(),
                           Bytes{frameMemoryArena_->statDeallocatedSize()}, frameMemoryArena_->statDeallocatedCount());

#if defined(NGN_ENABLE_INSTRUMENTATION)
            break;
#else
            frameCount = 0.0;
#endif
        }
        else
        {
            frameCount += 1.0;
        }
    }

    NGN_INSTRUMENTATION_MAIN_STOP();

    renderer_->waitForDevice();

#if defined(NGN_ENABLE_INSTRUMENTATION)
    ngn::instrumentation::dumpTimerInfos(std::cout);
#endif

    return exitCode_;
}

void Application::update(float deltaTime)
{
    NGN_INSTRUMENT_FUNCTION();

    stage_->onUpdate(deltaTime);

    world_->update(deltaTime);
}

void Application::draw(float deltaTime)
{
    NGN_UNUSED(deltaTime);
    NGN_INSTRUMENT_FUNCTION();

    const auto imageIndex = renderer_->startFrame();
    if (imageIndex == ngn::InvalidIndex)
        return;

    auto* commandBuffer = renderer_->currentCommandBuffer();

    commandBuffer->begin(imageIndex);

    if (spriteRenderer_)
        spriteRenderer_->draw(commandBuffer);

    if (uiRenderer_)
        uiRenderer_->draw(commandBuffer);

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
    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    app->renderer_->triggerFramebufferResized();
    app->stage_->onWindowResize(glm::vec2(width, height));
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    NGN_UNUSED(scancode);

    auto* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    app->stage_->onKeyEvent(toInputAction(action), key, toInputMods(mods));
}

} // namespace ngn

NGN_INSTRUMENTATION_EPILOG(Application)
