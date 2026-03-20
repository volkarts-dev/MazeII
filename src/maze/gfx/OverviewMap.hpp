// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include "gfx/Uniforms.hpp"
#include <entt/fwd.hpp>
#include <vulkan/vulkan.hpp>

namespace ngn {
class Buffer;
class CommandBuffer;
class Image;
class ImageView;
class Pipeline;
class Renderer;
class RenderTarget;
} // namespace ngn

class GameStage;

class OverviewMap
{
public:
    OverviewMap(GameStage* gameStage, glm::u32vec2 mapSize, uint32_t maxObjectCount);
    ~OverviewMap();

    ngn::RenderTarget* renderTarget() const { return renderTarget_; }
    const ngn::ImageView* mapImageView(uint32_t frameIndex) const { return mapImageViews_[frameIndex]; }

    void renderPoints();

    void draw(ngn::CommandBuffer* commandBuffer);

private:
    void createRenderObjects();

private:
    class UniformBuffer
    {
    public:
        ngn::Buffer* buffer;
        std::span<ngn::ViewProjection> mapped;
    };

    class Batch
    {
    public:
        ngn::Buffer* buffer;
        std::span<class OverviewMapVertex> mapped;
        uint32_t count;
    };

private:
    GameStage* gameStage_;
    ngn::Renderer* renderer_;
    entt::registry* registry_;
    glm::u32vec2 mapSize_;
    uint32_t maxObjectCount_;
    std::vector<ngn::Image*> mapImages_;
    std::vector<ngn::ImageView*> mapImageViews_;
    ngn::RenderTarget* renderTarget_;
    ngn::Pipeline* pipeline_;
    std::array<UniformBuffer, ngn::MaxFramesInFlight> uniformBuffers_;
    std::array<Batch, ngn::MaxFramesInFlight> batches_;

    NGN_DISABLE_COPY_MOVE(OverviewMap)
};

// *********************************************************************************************************************

class OverviewMapVertex
{
public:
    glm::vec2 position;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};

    static auto description()
    {
        return std::pair{
            vk::VertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(OverviewMapVertex),
                .inputRate = vk::VertexInputRate::eVertex,
            },
            std::array{
                vk::VertexInputAttributeDescription{
                    .location = 0,
                    .binding = 0,
                    .format = vk::Format::eR32G32Sfloat,
                    .offset = offsetof(OverviewMapVertex, position),
                },
                vk::VertexInputAttributeDescription{
                    .location = 1,
                    .binding = 0,
                    .format = vk::Format::eR32G32B32A32Sfloat,
                    .offset = offsetof(OverviewMapVertex, color),
                },
            },
        };
    };
};
