// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include <entt/fwd.hpp>
#include <vector>

namespace ngn {
class Application;
} // namespace ngn

class Level
{
public:
    Level(ngn::Application* app);
    ~Level();

private:
    void createMaze();

private:
    ngn::Application* app_;
    entt::registry* registry_;

    std::vector<entt::entity> walls_;
    std::vector<entt::entity> sprites_;

    NGN_DISABLE_COPY_MOVE(Level)
};
