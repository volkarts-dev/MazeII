// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once


#include "Macros.hpp"
namespace ngn {

class Pipeline;
class Renderer;

class DebugPipeline
{
public:
    enum class Mode
    {
        Line,
        Fill,
    };

public:
    DebugPipeline(Renderer* renderer, Mode mode);
    ~DebugPipeline();

    Pipeline* pipeline() const { return pipeline_; }

    void updateDescriptorSet(vk::DescriptorBufferInfo bufferInfo, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0);
    vk::DescriptorSet descriptorSet(uint32_t frame);

private:
    Renderer* renderer_;
    Pipeline* pipeline_;
    Mode mode_;

    NGN_DISABLE_COPY_MOVE(DebugPipeline)
};

// *********************************************************************************************************************

class DebugVertex
{
public:
    glm::vec2 point;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};

    static auto description()
    {
        return std::pair{
            vk::VertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(DebugVertex),
                .inputRate = vk::VertexInputRate::eVertex,
            },
            std::array{
                vk::VertexInputAttributeDescription{
                    .location = 0,
                    .binding = 0,
                    .format = vk::Format::eR32G32Sfloat,
                    .offset = offsetof(DebugVertex, point),
                },
                vk::VertexInputAttributeDescription{
                    .location = 1,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = offsetof(DebugVertex, color),
                },
            },
        };
    };
};

} // namespace ngn
