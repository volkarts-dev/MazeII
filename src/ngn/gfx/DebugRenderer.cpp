// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "DebugRenderer.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "DebugPipeline.hpp"
#include "Math.hpp"
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
            const auto val = TwoPI / static_cast<float>(values.size()) * static_cast<float>(i);
            values[i] = {std::cos(val), -std::sin(val)};
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

    const auto halfWidth = static_cast<float>(screenSize.width) / 2.0f;
    const auto halfHeight = static_cast<float>(screenSize.height) / 2.0f;
    ubo.mapped[0].proj = glm::ortho(
                -halfWidth, halfWidth,
                -halfHeight, halfHeight,
                -1.0f, 1.0f
                );
}

void DebugRenderer::drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4 color)
{
    auto& batch = lineBatches_[renderer_->currentFrame()];

    assert(batch.count + 1 < batch.buffer->size() / sizeof(DebugVertex));

    DebugVertex v = {start, color};
    batch.mapped[batch.count] = v;
    batch.count++;

    v.point = end;
    batch.mapped[batch.count] = v;
    batch.count++;
}

void DebugRenderer::drawArrow(const glm::vec2& start, const glm::vec2& end, float size, const glm::vec4 color)
{
    const auto ab = end - start;
    const auto len = glm::length(ab);
    const auto dir = ab / len;
    const auto base = start + dir * (len - size);
    const auto p1 = base + glm::vec2{-dir.y, dir.x} * size;
    const auto p2 = base - glm::vec2{-dir.y, dir.x} * size;

    drawLine(start, end, color);
    drawLine(end, p1, color);
    drawLine(end, p2, color);
}

void DebugRenderer::drawTriangle(const glm::vec2& edge1, const glm::vec2& edge2, const glm::vec2& edge3,
                                 const glm::vec4 color)
{
    drawLine(edge1, edge2, color);
    drawLine(edge2, edge3, color);
    drawLine(edge3, edge1, color);
}

void DebugRenderer::drawCircle(const glm::vec2& center, float radius, const glm::vec4 color)
{
    for (std::size_t i = 0; i < gCircleValues.size(); i++)
    {
        const auto idx0 = i;
        const auto idx1 = (i + 1) % gCircleValues.size();

        auto pos0 = center + gCircleValues[idx0] * radius;
        auto pos1 = center + gCircleValues[idx1] * radius;

        drawLine(pos0, pos1, color);
    }
}

void DebugRenderer::drawCapsule(const glm::vec2& start, const glm::vec2& end, float radius, const glm::vec4 color)
{
    const auto ab = end - start;
    const auto perp = glm::vec2{ab.y, -ab.x};
    const auto norm = perp / glm::length(perp);

    const auto start2 = start + norm * radius;
    const auto end2 = end + norm * radius;

    const auto start3 = start - norm * radius;
    const auto end3 = end - norm * radius;

    drawLine(start2, end2, color);
    drawLine(start3, end3, color);

    auto theta = atan2(-norm.y, norm.x);

    std::size_t startIndex = 0;
    for ( ; startIndex < gCircleValues.size(); startIndex++)
    {
        const auto t = TwoPI / static_cast<float>(gCircleValues.size()) * static_cast<float>(startIndex);
        if (t > theta)
            break;
    }

    glm::vec2 r0 = start2;
    glm::vec2 r1;
    std::size_t i = startIndex;

    for ( ; i < startIndex + gCircleValues.size() / 2; i++)
    {
        r1 = start + gCircleValues[i % gCircleValues.size()] * radius;
        drawLine(r0, r1, color);
        r0 = r1;
    }

    r1 = start3;
    drawLine(r0, r1, color);
    r0 = end3;

    for ( ; i < startIndex + gCircleValues.size(); i++)
    {
        r1 = end + gCircleValues[i % gCircleValues.size()] * radius;
        drawLine(r0, r1, color);
        r0 = r1;
    }

    r1 = end2;
    drawLine(r0, r1, color);
}

void DebugRenderer::drawAABB(const glm::vec2& topLeft, const glm::vec2& bottomRight, const glm::vec4 color)
{
    glm::vec2 pos0 = topLeft;
    glm::vec2 pos1 = {bottomRight.x, topLeft.y};
    drawLine(pos0, pos1, color);

    pos0 = pos1;
    pos1 = bottomRight;
    drawLine(pos0, pos1, color);

    pos0 = pos1;
    pos1 = {topLeft.x, bottomRight.y};
    drawLine(pos0, pos1, color);

    pos0 = pos1;
    pos1 = topLeft;
    drawLine(pos0, pos1, color);
}

void DebugRenderer::fillTriangle(const glm::vec2& edge1, const glm::vec2& edge2, const glm::vec2& edge3,
                                 const glm::vec4 color)
{
    auto& batch = triangleBatches_[renderer_->currentFrame()];

    assert(batch.count + 2 < batch.buffer->size() / sizeof(DebugVertex));

    DebugVertex v = {edge1, color};
    batch.mapped[batch.count] = v;
    batch.count++;

    v.point = edge2;
    batch.mapped[batch.count] = v;
    batch.count++;

    v.point = edge3;
    batch.mapped[batch.count] = v;
    batch.count++;
}

void DebugRenderer::fillCircle(const glm::vec2& center, float radius, const glm::vec4 color)
{
    for (std::size_t i = 0; i < gCircleValues.size(); i++)
    {
        const auto idx0 = i;
        const auto idx1 = (i + 1) % gCircleValues.size();

        auto pos0 = center + gCircleValues[idx0] * radius;
        auto pos1 = center + gCircleValues[idx1] * radius;

        fillTriangle(center, pos1, pos0, color);
    }
}

void DebugRenderer::fillCapsule(const glm::vec2& start, const glm::vec2& end, float radius, const glm::vec4 color)
{
    const auto ab = end - start;
    const auto perp = glm::vec2{ab.y, -ab.x};
    const auto norm = perp / glm::length(perp);

    const auto start2 = start + norm * radius;
    const auto end2 = end + norm * radius;

    const auto start3 = start - norm * radius;
    const auto end3 = end - norm * radius;

    fillTriangle(start2, end2, end3, color);
    fillTriangle(start2, end3, start3, color);

    auto theta = atan2(-norm.y, norm.x);

    std::size_t startIndex = 0;
    for ( ; startIndex < gCircleValues.size(); startIndex++)
    {
        const auto t = TwoPI / static_cast<float>(gCircleValues.size()) * static_cast<float>(startIndex);
        if (t > theta)
            break;
    }

    glm::vec2 r0 = start2;
    glm::vec2 r1;
    std::size_t i = startIndex;

    for ( ; i < startIndex + gCircleValues.size() / 2; i++)
    {
        r1 = start + gCircleValues[i % gCircleValues.size()] * radius;
        fillTriangle(start, r1, r0, color);
        r0 = r1;
    }

    r1 = start3;
    fillTriangle(start, r1, r0, color);
    r0 = end3;

    for ( ; i < startIndex + gCircleValues.size(); i++)
    {
        r1 = end + gCircleValues[i % gCircleValues.size()] * radius;
        fillTriangle(end, r1, r0, color);
        r0 = r1;
    }

    r1 = end2;
    fillTriangle(end, r1, r0, color);
}

void DebugRenderer::fillAABB(const glm::vec2& topLeft, const glm::vec2& bottomRight, const glm::vec4 color)
{
    glm::vec2 pos0 = topLeft;
    glm::vec2 pos1 = {bottomRight.x, topLeft.y};
    glm::vec2 pos2 = bottomRight;
    glm::vec2 pos3 = {topLeft.x, bottomRight.y};

    fillTriangle(pos0, pos1, pos2, color);

    fillTriangle(pos2, pos1, pos3, color);
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
