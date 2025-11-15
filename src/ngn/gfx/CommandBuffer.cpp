// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "CommandBuffer.hpp"

#include "Buffer.hpp"
#include "Renderer.hpp"
#include "Pipeline.hpp"

namespace ngn {

CommandBufferConfig::CommandBufferConfig(Renderer* _renderer) :
    renderer{_renderer}
{
}

// *********************************************************************************************************************

CommandBuffer::CommandBuffer(const CommandBufferConfig& config) :
    renderer_{config.renderer}
{
    vk::CommandBufferAllocateInfo allocateInfo{
        .commandPool = renderer_->commandPool(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };

    commandBuffer_ = renderer_->device().allocateCommandBuffers(allocateInfo)[0];
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::begin(uint32_t imageIndex)
{
    commandBuffer_.reset();

    vk::CommandBufferBeginInfo beginInfo{
    };

    commandBuffer_.begin(beginInfo);

    vk::ClearValue clearColor = {
        .color = {{{0.0f, 0.0f, 0.0f, 1.0f}}},
    };

    vk::RenderPassBeginInfo renderPassBeginInfo{
        .renderPass = renderer_->renderPass(),
        .framebuffer = renderer_->swapChainFramebuffer(imageIndex),
        .renderArea = {{0, 0}, renderer_->swapChainExtent()},

    };
    renderPassBeginInfo.setClearValues(clearColor);

    commandBuffer_.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    vk::Viewport viewport{
        0.0f, 0.0f,
        static_cast<float>(renderer_->swapChainExtent().width),
        static_cast<float>(renderer_->swapChainExtent().height),
        0.0f, 1.0f,
    };
    commandBuffer_.setViewport(0, viewport);

    vk::Rect2D scissor{
        {0, 0},
        renderer_->swapChainExtent(),
    };
    commandBuffer_.setScissor(0, scissor);
}

void CommandBuffer::end()
{
    commandBuffer_.endRenderPass();
    commandBuffer_.end();
}

void CommandBuffer::bindPipeline(Pipeline* pipeline)
{
    commandBuffer_.bindPipeline(pipeline->bindPoint(), pipeline->handle());
}

void CommandBuffer::bindDescriptorSet(Pipeline* pipeline, vk::DescriptorSet descriptorSet)
{
    commandBuffer_.bindDescriptorSets(pipeline->bindPoint(), pipeline->layout(), 0, descriptorSet, {});
}

void CommandBuffer::bindVertexBuffer(Buffer* buffer)
{
    commandBuffer_.bindVertexBuffers(0, buffer->handle(), {0});
}

void CommandBuffer::draw(uint32_t vertexCount)
{
    commandBuffer_.draw(vertexCount, 1, 0, 0);
}

void CommandBuffer::copyBuffer(Buffer* src, Buffer* dest, uint32_t size, uint32_t srcOff, uint32_t dstOff)
{
    vk::BufferCopy copyRegion{
        .srcOffset = srcOff,
        .dstOffset = dstOff,
        .size = size,
    };
    commandBuffer_.copyBuffer(src->handle(), dest->handle(), copyRegion);
}

} // namespace ngn
