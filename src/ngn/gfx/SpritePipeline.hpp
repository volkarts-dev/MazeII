// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Uniforms.hpp"
#include <glm/glm.hpp>

namespace ngn {

class Buffer;
class CommandBuffer;
class Pipeline;
class Renderer;

struct SpriteUniform
{
    ViewProjection viewProj;
};

class SpritePipeline
{
public:
    SpritePipeline(Renderer* renderer);
    ~SpritePipeline();

    Pipeline* pipeline() const { return pipeline_; }

    void updateDescriptorSet(vk::DescriptorBufferInfo bufferInfo, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0);
    void updateDescriptorSet(vk::DescriptorImageInfo imageInfo, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0);
    vk::DescriptorSet descriptorSet(uint32_t frame);


private:
    Renderer* renderer_;
    Pipeline* pipeline_;

    NGN_DISABLE_COPY_MOVE(SpritePipeline)
};

// *********************************************************************************************************************

class SpriteVertex
{
public:
    glm::vec2 position;
    float rotation;
    glm::vec2 scale;
    glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
    uint32_t texIndex;
    glm::vec4 texCoords;

    static auto description()
    {
        return std::pair{
            vk::VertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(SpriteVertex),
                .inputRate = vk::VertexInputRate::eVertex,
            },
            std::array{
                vk::VertexInputAttributeDescription{
                    .location = 0,
                    .binding = 0,
                    .format = vk::Format::eR32G32Sfloat,
                    .offset = offsetof(SpriteVertex, position),
                },
                vk::VertexInputAttributeDescription{
                    .location = 1,
                    .binding = 0,
                    .format = vk::Format::eR32Sfloat,
                    .offset = offsetof(SpriteVertex, rotation),
                },
                vk::VertexInputAttributeDescription{
                    .location = 2,
                    .binding = 0,
                    .format = vk::Format::eR32G32Sfloat,
                    .offset = offsetof(SpriteVertex, scale),
                },
                vk::VertexInputAttributeDescription{
                    .location = 3,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = offsetof(SpriteVertex, color),
                },
                vk::VertexInputAttributeDescription{
                    .location = 4,
                    .binding = 0,
                    .format = vk::Format::eR32Uint,
                    .offset = offsetof(SpriteVertex, texIndex),
                },
                vk::VertexInputAttributeDescription{
                    .location = 5,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = offsetof(SpriteVertex, texCoords),
                },
            },
        };
    };
};

} // namespace ngn
