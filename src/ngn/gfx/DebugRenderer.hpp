// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Colors.hpp"
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

    void drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4 color = Colors::White);
    void drawArrow(const glm::vec2& start, const glm::vec2& end, float size, const glm::vec4 color = Colors::White);
    void drawTriangle(const glm::vec2& edge1, const glm::vec2& edge2, const glm::vec2& edge3,
                      const glm::vec4 color = Colors::White);
    void drawCircle(const glm::vec2& center, float radius, const glm::vec4 color = Colors::White);
    void drawCapsule(const glm::vec2& start, const glm::vec2& end, float radius, const glm::vec4 color = Colors::White);
    void drawAABB(const glm::vec2& topLeft, const glm::vec2& bottomRight, const glm::vec4 color = Colors::White);

    void fillTriangle(const glm::vec2& edge1, const glm::vec2& edge2, const glm::vec2& edge3,
                      const glm::vec4 color = Colors::White);
    void fillCircle(const glm::vec2& center, float radius, const glm::vec4 color = Colors::White);
    void fillCapsule(const glm::vec2& start, const glm::vec2& end, float radius, const glm::vec4 color = Colors::White);
    void fillAABB(const glm::vec2& topLeft, const glm::vec2& bottomRight, const glm::vec4 color = Colors::White);

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
