// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "DebugRenderer.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "DebugPipeline.hpp"
#include "Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace ngn {

namespace {

struct CircleValues
{
    CircleValues()
    {
        for (std::size_t i = 0; i < values.size(); i++)
        {
            const auto val = glm::pi<float>() * 2.f / static_cast<float>(values.size()) * static_cast<float>(i);
            values[i] = {std::sin(val), std::cos(val)};
        }
    }

    auto size() const { return values.size(); }

    auto operator[](std::size_t i) { return values[i]; }

    std::array<glm::vec2, 16> values{};
};
CircleValues gCircleValues;

} // namespace

DebugRenderer::DebugRenderer(Renderer* renderer, uint32_t batchSize) :
    renderer_{renderer},
    fillPipeline_{new DebugPipeline{renderer_, DebugPipeline::Mode::Fill}},
    linePipeline_{new DebugPipeline{renderer_, DebugPipeline::Mode::Line}}
{
    BufferConfig uniformBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eUniformBuffer,
        sizeof(ViewProjection)
    };
    uniformBufferConfig.hostVisible = true;

    // TODO Use one buffer for all uniforms
    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        uniformBuffers_[i].buffer = new Buffer{uniformBufferConfig};
        uniformBuffers_[i].mapped = uniformBuffers_[i].buffer->map<ViewProjection>();

        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers_[i].buffer->handle(),
            .offset = 0,
            .range = sizeof(ViewProjection),
        };
        fillPipeline_->updateDescriptorSet(bufferInfo, i, 0);
        linePipeline_->updateDescriptorSet(bufferInfo, i, 0);
    }

    // ****************************************************

    ngn::BufferConfig spriteBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eVertexBuffer,
        sizeof(ngn::DebugVertex) * batchSize
    };
    spriteBufferConfig.hostVisible = true;
    for (uint32_t f = 0; f < MaxFramesInFlight; f++)
    {
        lineBatches_[f].buffer = new ngn::Buffer{spriteBufferConfig};
        lineBatches_[f].mapped = lineBatches_[f].buffer->map<DebugVertex>();
        lineBatches_[f].count = 0;

        triangleBatches_[f].buffer = new ngn::Buffer{spriteBufferConfig};
        triangleBatches_[f].mapped = triangleBatches_[f].buffer->map<DebugVertex>();
        triangleBatches_[f].count = 0;
    }
}

DebugRenderer::~DebugRenderer()
{
    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        triangleBatches_[f].buffer->unmap();
        delete triangleBatches_[f].buffer;

        lineBatches_[f].buffer->unmap();
        delete lineBatches_[f].buffer;
    }

    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        uniformBuffers_[f].buffer->unmap();
        delete uniformBuffers_[f].buffer;
    }

    delete linePipeline_;
    delete fillPipeline_;
}

void DebugRenderer::updateView(const glm::mat4& view)
{
    const auto screenSize = renderer_->swapChainExtent();

    auto& ubo = uniformBuffers_[renderer_->currentFrame()];

    ubo.mapped[0].view = view;
    ubo.mapped[0].proj = glm::ortho(
                0.0f, static_cast<float>(screenSize.width),
                0.0f, static_cast<float>(screenSize.height)
                );
}

void DebugRenderer::drawLine(const DebugVertex& start, const DebugVertex& end)
{
    auto& batch = lineBatches_[renderer_->currentFrame()];

    assert(batch.count + 1 < batch.buffer->size() / sizeof(DebugVertex));

    std::memcpy(&batch.mapped[batch.count], &start, sizeof(DebugVertex));
    batch.count++;

    std::memcpy(&batch.mapped[batch.count], &end, sizeof(DebugVertex));
    batch.count++;
}

void DebugRenderer::drawTriangle(const DebugVertex& edge1, const DebugVertex& edge2, const DebugVertex& edge3)
{
    drawLine(edge1, edge2);
    drawLine(edge2, edge3);
    drawLine(edge3, edge1);
}

void DebugRenderer::drawCircle(const DebugVertex& center, float radius)
{
    for (std::size_t i = 0; i < gCircleValues.size(); i++)
    {
        const auto idx0 = i;
        const auto idx1 = (i + 1) % gCircleValues.size();

        auto pos0 = center;
        pos0.point += gCircleValues[idx0] * radius;
        auto pos1 = center;
        pos1.point += gCircleValues[idx1] * radius;

        drawLine(pos0, pos1);
    }
}

void DebugRenderer::fillTriangle(const DebugVertex& edge1, const DebugVertex& edge2, const DebugVertex& edge3)
{
    auto& batch = triangleBatches_[renderer_->currentFrame()];

    assert(batch.count + 2 < batch.buffer->size() / sizeof(DebugVertex));

    std::memcpy(&batch.mapped[batch.count], &edge1, sizeof(DebugVertex));
    batch.count++;

    std::memcpy(&batch.mapped[batch.count], &edge2, sizeof(DebugVertex));
    batch.count++;

    std::memcpy(&batch.mapped[batch.count], &edge3, sizeof(DebugVertex));
    batch.count++;
}

void DebugRenderer::fillCircle(const DebugVertex& center, float radius)
{
    for (std::size_t i = 0; i < gCircleValues.size(); i++)
    {
        const auto idx0 = i;
        const auto idx1 = (i + 1) % gCircleValues.size();

        auto pos0 = center;
        pos0.point += gCircleValues[idx0] * radius;
        auto pos1 = center;
        pos1.point += gCircleValues[idx1] * radius;

        fillTriangle(center, pos1, pos0);
    }
}

void DebugRenderer::draw(CommandBuffer* commandBuffer)
{
    const auto frameIndex = renderer_->currentFrame();

    if (auto& triangleBatch = triangleBatches_[frameIndex]; triangleBatch.count > 0)
    {
        commandBuffer->bindPipeline(fillPipeline_->pipeline());
        commandBuffer->bindDescriptorSet(fillPipeline_->pipeline(), fillPipeline_->descriptorSet(frameIndex));
        commandBuffer->bindVertexBuffer(triangleBatch.buffer);
        commandBuffer->draw(triangleBatch.count);

        triangleBatch.count = 0;
    }

    if (auto& lineBatch = lineBatches_[frameIndex]; lineBatch.count > 0)
    {
        commandBuffer->bindPipeline(linePipeline_->pipeline());
        commandBuffer->bindDescriptorSet(linePipeline_->pipeline(), linePipeline_->descriptorSet(frameIndex));
        commandBuffer->bindVertexBuffer(lineBatch.buffer);
        commandBuffer->draw(lineBatch.count);

        lineBatch.count = 0;
    }
}

} // namespace ngn
