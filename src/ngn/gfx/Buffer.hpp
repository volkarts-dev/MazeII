// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include <vulkan/vulkan.hpp>

namespace ngn {

class Renderer;

class BufferConfig
{
public:
    BufferConfig(Renderer* _renderer, vk::BufferUsageFlags _usage, std::size_t _size);

    Renderer* const renderer;
    const vk::BufferUsageFlags usage;
    const std::size_t size;
    bool hostVisible{};
};

class Buffer
{
public:
    Buffer(const BufferConfig& config);
    ~Buffer();

    const vk::Buffer& handle() const { return buffer_; }
    std::size_t size() const { return size_; }

    template<typename T>
    inline std::span<T> map()
    {
        auto area = map();
        return {reinterpret_cast<T*>(area.data()), area.size() / sizeof(T)};
    }

    BufferView map();
    void unmap();

private:
    Renderer* renderer_;
    vk::Device device_;
    vk::Buffer buffer_;
    vk::DeviceMemory memory_;
    std::size_t size_;

    NGN_DISABLE_COPY_MOVE(Buffer)
};

} // namespace ngn

