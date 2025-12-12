// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "CommonComponents.hpp"
#include "gfx/Renderer.hpp"
#include "Macros.hpp"
#include <entt/fwd.hpp>

struct GLFWwindow;

namespace ngn {

class Application;
class LinearAllocator;
class World;

class ApplicationDelegate
{
public:
    virtual ~ApplicationDelegate() = default;

    virtual std::size_t requiredFrameMemeory() = 0;
    virtual bool onInit(Application* app) { NGN_UNUSED(app); return true; };
    virtual void onDone(Application* app) { NGN_UNUSED(app); };

    virtual void onKeyEvent(ngn::Application* app, int action, int key) {  NGN_UNUSED(app); NGN_UNUSED(action); NGN_UNUSED(key); }

    virtual void onUpdate(Application* app, float deltaTime) { NGN_UNUSED(app); NGN_UNUSED(deltaTime); };
    virtual void onDraw(Application* app, float deltaTime) { NGN_UNUSED(app); NGN_UNUSED(deltaTime); };
};

class Application
{
public:
    static Application* get();

public:
    Application(ApplicationDelegate* delegate);
    ~Application();

    Renderer* renderer() const { return renderer_; }
    LinearAllocator* frameAllocator() const { return frameAllocator_; }
    entt::registry* registry() const { return registry_; }
    World* world() const { return world_; }

    entt::entity createActor(Position pos, Rotation rot = {}, Scale sca= {});

    bool isKeyDown(int key) const;
    bool isKeyUp(int key) const;

    void exec();

private:
    void update(float deltaTime);
    void draw(float deltaTime);

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    void handleKeyPress(int key, int scancode, int mods);
    void handleKeyRelease(int key, int scancode, int mods);

private:
    ApplicationDelegate* delegate_;
    GLFWwindow* window_;
    Renderer* renderer_;
    LinearAllocator* frameAllocator_;
    entt::registry* registry_;
    World* world_;

    NGN_DISABLE_COPY_MOVE(Application)
};

} // namespace ngn
