// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "DebugPipeline.hpp"
#include "Types.hpp"
#include "Uniforms.hpp"

namespace ngn {

class Buffer;
class CommandBuffer;
class Renderer;

class DebugRenderer
{
public:
    DebugRenderer(Renderer* renderer, uint32_t batchSize);
    ~DebugRenderer();

    void updateView(const glm::mat4& view);

    void drawLine(const DebugVertex& start, const DebugVertex& end);
    void drawTriangle(const DebugVertex& edge1, const DebugVertex& edge2, const DebugVertex& edge3);
    void drawCircle(const DebugVertex& center, float radius);

    void fillTriangle(const DebugVertex& edge1, const DebugVertex& edge2, const DebugVertex& edge3);
    void fillCircle(const DebugVertex& center, float radius);

    void draw(CommandBuffer* commandBuffer);

private:
    struct UniformBuffer
    {
        Buffer* buffer;
        std::span<ViewProjection> mapped;
    };

    struct Batch
    {
        Buffer* buffer;
        std::span<DebugVertex> mapped;
        uint32_t count;
    };

private:
    Renderer* renderer_;
    DebugPipeline* fillPipeline_;
    DebugPipeline* linePipeline_;
    std::array<UniformBuffer, MaxFramesInFlight> uniformBuffers_;
    std::array<Batch, MaxFramesInFlight> lineBatches_;
    std::array<Batch, MaxFramesInFlight> triangleBatches_;

    NGN_DISABLE_COPY_MOVE(DebugRenderer)
};

} // namespace ngn
