// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include <vulkan/vulkan.hpp>

namespace ngn {

class Buffer;
class Renderer;
class Pipeline;

class CommandBufferConfig
{
public:
    CommandBufferConfig(Renderer* _renderer);

    Renderer* const renderer;
};

class CommandBuffer
{
public:
    CommandBuffer(const CommandBufferConfig& config);
    ~CommandBuffer();

    vk::CommandBuffer handle() const { return commandBuffer_; }

    void begin(uint32_t imageIndex);
    void end();

    void bindPipeline(Pipeline* pipeline);
    void bindDescriptorSet(Pipeline* pipeline, vk::DescriptorSet descriptorSet);
    void bindVertexBuffer(Buffer* buffer);
    void draw(uint32_t vertexCount);

    void copyBuffer(Buffer* src, Buffer* dest, uint32_t size, uint32_t srcOff, uint32_t dstOff);

private:
    Renderer* renderer_;
    vk::CommandBuffer commandBuffer_;

    NGN_DISABLE_COPY_MOVE(CommandBuffer)
};

} // namespace ngn
