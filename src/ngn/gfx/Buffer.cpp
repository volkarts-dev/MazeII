// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Buffer.hpp"

#include "Renderer.hpp"

namespace ngn {

BufferConfig::BufferConfig(Renderer* _renderer, vk::BufferUsageFlags _usage, std::size_t _size) :
    renderer{_renderer},
    usage{_usage},
    size{_size}
{
}

Buffer::Buffer(const BufferConfig& config) :
    renderer_{config.renderer},
    device_{config.renderer->device()},
    size_{config.size}
{
    vk::BufferCreateInfo createInfo{
        .size = config.size,
        .usage = config.usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };

    buffer_ = device_.createBuffer(createInfo);

    const auto requirements = device_.getBufferMemoryRequirements(buffer_);

    vk::MemoryPropertyFlags memoryFlags = {};
    if (config.hostVisible)
    {
        memoryFlags |= vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    }

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = requirements.size,
        .memoryTypeIndex = renderer_->findMemoryType(requirements.memoryTypeBits, memoryFlags),
    };

    memory_ = device_.allocateMemory(allocInfo);

    device_.bindBufferMemory(buffer_, memory_, 0);
}

Buffer::~Buffer()
{
    device_.destroyBuffer(buffer_);
    device_.freeMemory(memory_);
}

BufferView Buffer::map()
{
    auto* ptr = reinterpret_cast<uint8_t*>(device_.mapMemory(memory_, 0, size_));
    return {ptr, size_};
}

void Buffer::unmap()
{
    device_.unmapMemory(memory_);
}

} // namespace ngn
