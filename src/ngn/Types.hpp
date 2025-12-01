// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <span>
#include <cstdint>
#include <limits>
#include <chrono>

namespace ngn {

using Clock = std::chrono::high_resolution_clock;
using Timpoint = Clock::time_point;
template<typename Rep>
using Duration = std::chrono::duration<Rep>;

using BufferView = std::span<uint8_t>;

constexpr auto InvalidIndex = std::numeric_limits<uint32_t>::max();

constexpr uint32_t MaxFramesInFlight = 2;
constexpr uint32_t MaxSpritePipelineTextures = 8;

} // namespace ngn
