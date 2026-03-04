// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

// IWYU pragma: begin_exports

#include <limits>
#include <span>
#include <string_view>
#include <cstdint>

// IWYU pragma: end_exports

using namespace std::literals;

namespace ngn {

using BufferView = std::span<uint8_t>;

template<std::unsigned_integral T = uint32_t>
constexpr auto InvalidIndex = std::numeric_limits<T>::max();

constexpr auto InvalidIndex16 = InvalidIndex<uint16_t>;
constexpr auto InvalidIndex32 = InvalidIndex<uint32_t>;

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
