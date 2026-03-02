// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include <entt/fwd.hpp>
#include <vector>

namespace ngn {

class Application;
class NavigationGraph;

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
class DebugRenderer;
#endif

} // namespace ngn

class Board
{
public:
    Board(ngn::Application* app);
    ~Board();

    ngn::NavigationGraph* navigationGraph() const { return navigationGraph_; }

#if defined(NGN_ENABLE_VISUAL_DEBUGGING)
    void debugDrawState(ngn::DebugRenderer* debugRenderer);
#endif

private:
    void createWalls();
    void createSprites();
    void createNavigationGraph();

private:
    ngn::Application* app_;
    entt::registry* registry_;

    std::vector<entt::entity> walls_;
    std::vector<entt::entity> sprites_;

    ngn::NavigationGraph* navigationGraph_;

    NGN_DISABLE_COPY_MOVE(Board)
};
