// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <span>
#include <cstdint>
#include <limits>
#include <chrono>

namespace ngn {

using Clock = std::chrono::high_resolution_clock;
using Timepoint = Clock::time_point;
template<typename Rep>
using Duration = std::chrono::duration<Rep>;

using BufferView = std::span<uint8_t>;

constexpr auto InvalidIndex = std::numeric_limits<uint32_t>::max();

constexpr uint32_t MaxFramesInFlight = 2;
constexpr uint32_t MaxSpritePipelineTextures = 8;

template<typename _Tp, typename _Up>
concept not_same_as = !std::is_same_v<_Tp, _Up>;

template<typename Func, typename... Args>
concept ReturnsVoid = requires(Func func, Args... args)
{
    { func(std::forward<Args>(args)...) } -> std::same_as<void>;
};

template<typename Func, typename... Args>
concept ReturnsNonVoid = requires(Func func, Args... args)
{
    { func(std::forward<Args>(args)...) } -> not_same_as<void>;
};

} // namespace ngn
