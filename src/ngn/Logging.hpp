// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <spdlog/spdlog.h>

namespace ngn::log {

namespace level = spdlog::level;

using spdlog::set_level;

using spdlog::critical;
using spdlog::error;
using spdlog::warn;
using spdlog::info;
using spdlog::debug;
using spdlog::trace;

} // namespace ngn::log
