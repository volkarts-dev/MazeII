// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Allocators.hpp"
#include "CommonComponents.hpp"
#include "Input.hpp"
#include "gfx/Renderer.hpp"
#include "Macros.hpp"
#include <entt/fwd.hpp>

struct GLFWwindow;

namespace ngn {

class Application;
class Audio;
class UiRenderer;
class FontMaker;
class MemoryArena;
class SpriteRenderer;
class World;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
class DebugRenderer;
#endif

// *********************************************************************************************************************

class ApplicationConfig
{
public:
    int windowWidth{};
    int windowHeight{};
    const char* windowTitle{};

    std::size_t requiredMemory{};

    bool spriteRenderer{};
    uint32_t spriteBatchCount{};

    bool fontRenderer{};
    uint32_t fontBatchCount{};

    bool audio{};

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    bool debugRenderer{};
    uint32_t debugBatchCount{};
#endif
};

// *********************************************************************************************************************

class ApplicationStage
{
public:
    virtual ~ApplicationStage();

    virtual void onActivate() { }
    virtual void onDeactivate() { }

    virtual void onWindowResize(const glm::vec2& windowSize) { NGN_UNUSED(windowSize); }
    virtual void onKeyEvent(InputAction action, int key, InputMods mods) { NGN_UNUSED(action); NGN_UNUSED(key); NGN_UNUSED(mods); }
    virtual void onUpdate(float deltaTime) { NGN_UNUSED(deltaTime); }
};

// *********************************************************************************************************************

class ApplicationDelegate
{
public:
    virtual ~ApplicationDelegate();

    virtual ApplicationConfig applicationConfig(Application* app) = 0;

    virtual ApplicationStage* onInit(Application* app) = 0;
    virtual void onDone(Application* app) { NGN_UNUSED(app); };
};

// *********************************************************************************************************************

class Application
{
public:
    Application(ApplicationDelegate* delegate);
    ~Application();

    glm::vec2 windowSize() const;

    Renderer* renderer() const { return renderer_; }
    MemoryArena* frameMemoryArena() const { return frameMemoryArena_; }
    entt::registry* registry() const { return registry_; }
    World* world() const { return world_; }

    SpriteRenderer* spriteRenderer() const { return spriteRenderer_; }
    UiRenderer* uiRenderer() const { return uiRenderer_; }
#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    DebugRenderer* debugRenderer() const { return debugRenderer_; }
#endif
    Audio* audio() const { return audio_; }

    void activateStage(ApplicationStage* stage);
    void quit(int exitCode = 0);

    template<typename T>
    LinearAllocator<T> createFrameAllocator()
    {
        return LinearAllocator<T>{frameMemoryArena_};
    }

    entt::entity createActor(glm::vec2 pos, float rot = 0.0f, glm::vec2 sca = {1, 1}, bool active = true);

    bool isKeyDown(int key) const;
    bool isKeyUp(int key) const;

    int exec();

private:
    void update(float deltaTime);
    void draw(float deltaTime);

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    ApplicationDelegate* delegate_;
    GLFWwindow* window_;
    Renderer* renderer_;
    MemoryArena* frameMemoryArena_;

    SpriteRenderer* spriteRenderer_;
    UiRenderer* uiRenderer_;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    DebugRenderer* debugRenderer_;
#endif

    Audio* audio_;

    entt::registry* registry_;
    World* world_;

    ApplicationStage* stage_;
    ApplicationStage* nextStage_;

    int exitCode_;

    NGN_DISABLE_COPY_MOVE(Application)
};

} // namespace ngn
