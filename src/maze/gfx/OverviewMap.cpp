// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "OverviewMap.hpp"

#include "Application.hpp"
#include "Board.hpp"
#include "CommonComponents.hpp"
#include "GameStage.hpp"
#include "MazeAssets.hpp"
#include "MazeComponents.hpp"
#include "Types.hpp"
#include "gfx/Buffer.hpp"
#include "gfx/CommandBuffer.hpp"
#include "gfx/Image.hpp"
#include "gfx/Pipeline.hpp"
#include "gfx/RenderTarget.hpp"
#include "gfx/Renderer.hpp"
#include <entt/entt.hpp>
#include <glm/gtc/matrix_transform.hpp>

OverviewMap::OverviewMap(GameStage* gameStage, glm::u32vec2 mapSize, uint32_t maxObjectCount) :
    gameStage_{gameStage},
    renderer_{gameStage_->app()->renderer()},
    registry_{gameStage_->app()->registry()},
    mapSize_{mapSize},
    maxObjectCount_{maxObjectCount}
{
    createRenderObjects();
}

OverviewMap::~OverviewMap()
{
    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        batches_[f].buffer->unmap();
        delete batches_[f].buffer;
    }

    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        uniformBuffers_[f].buffer->unmap();
        delete uniformBuffers_[f].buffer;
    }

    delete pipeline_;

    delete renderTarget_;

    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        delete mapImageViews_[i];
        delete mapImages_[i];
    }
}

void OverviewMap::createRenderObjects()
{
    auto imageLoader = ngn::ImageLoader::createEmpty(renderer_, vk::Format::eB8G8R8A8Srgb, mapSize_.x, mapSize_.y);

    for (uint32_t i = 0; i < ngn::MaxFramesInFlight; i++)
    {
        auto* image = new ngn::Image{imageLoader};
        mapImages_.push_back(image);
        mapImageViews_.push_back(new ngn::ImageView{image});
    }

    ngn::RenderTargetCreateInfo createInfo{
        .renderer = renderer_,
        .imagesSize = vk::Extent2D{mapSize_.x, mapSize_.y},
        .imagesFormat = vk::Format::eB8G8R8A8Srgb,
        .finalImagesLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .colorAttachments = mapImageViews_,
        .dependencies = {
            vk::SubpassDependency{
                .srcSubpass = vk::SubpassExternal,
                .dstSubpass = 0,
                .srcStageMask = vk::PipelineStageFlagBits::eFragmentShader,
                .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .srcAccessMask = vk::AccessFlagBits::eNone,
                .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            },
            vk::SubpassDependency{
                .srcSubpass = 0,
                .dstSubpass = vk::SubpassExternal,
                .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                .dstStageMask = vk::PipelineStageFlagBits::eFragmentShader,
                .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
                .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            }
        },
        .clearColor = {0.01f, 0.01f, 0.01f, 1.0f},
    };

    renderTarget_ = new ngn::RenderTarget{createInfo};

    // ****************************************************

    ngn::PipelineConfig config{renderer_};

    config.renderTarget = renderTarget_;

    config.topology = vk::PrimitiveTopology::ePointList;

    config.vertexShaderCode = maze::assets::shader_OverviewMap_vert_spv();
    config.fragmentShaderCode = maze::assets::shader_OverviewMap_frag_spv();

    std::array descriptorSetLayout{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        },
    };

    config.descriptorSetLayout = descriptorSetLayout;

    auto vertexDescription = OverviewMapVertex::description();
    config.vertexBinding = vertexDescription.first;
    config.vertexAttributes = vertexDescription.second;

    config.blendEnabled = true;

    pipeline_ = new ngn::Pipeline{config};

    // ****************************************************

    ngn::BufferConfig uniformBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eUniformBuffer,
        sizeof(ngn::ViewProjection)
    };
    uniformBufferConfig.hostVisible = true;

    // TODO Use one buffer for all uniforms
    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        uniformBuffers_[f].buffer = new ngn::Buffer{uniformBufferConfig};
        uniformBuffers_[f].mapped = uniformBuffers_[f].buffer->map<ngn::ViewProjection>();

        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers_[f].buffer->handle(),
            .offset = 0,
            .range = sizeof(ngn::ViewProjection),
        };
        pipeline_->updateDescriptorSet(bufferInfo, f, 0);
    }

    // ****************************************************

    ngn::BufferConfig spriteBufferConfig{
        renderer_,
        vk::BufferUsageFlagBits::eVertexBuffer,
        sizeof(OverviewMapVertex) * maxObjectCount_
    };
    spriteBufferConfig.hostVisible = true;
    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        batches_[f].buffer = new ngn::Buffer{spriteBufferConfig};
        batches_[f].mapped = batches_[f].buffer->map<OverviewMapVertex>();
        batches_[f].count = 0;
    }

    // ****************************************************

    const auto boardSize = gameStage_->board()->dimension();
    const auto halfBoardSize = boardSize * 0.5f;

    const auto mapSize = glm::vec2{mapSize_};
    const auto mapHalfSize = mapSize * 0.5f * (boardSize / mapSize);

    for (uint32_t f = 0; f < ngn::MaxFramesInFlight; f++)
    {
        auto& ubo = uniformBuffers_[f];

        ubo.mapped[0].view = glm::lookAt(
            glm::vec3{halfBoardSize, 0.5f},
            glm::vec3{halfBoardSize, 0.0f},
            glm::vec3{0.0f, 1.0f, 0.0f}
        );
        ubo.mapped[0].proj = glm::ortho(
            -mapHalfSize.x, mapHalfSize.x,
            -mapHalfSize.y, mapHalfSize.y,
            -1.0f, 1.0f
        );
    }
}

void OverviewMap::renderPoints()
{
    auto& batch = batches_[renderer_->currentFrame()];

    auto player = registry_->view<const ngn::Position, PlayerTag, ngn::ActiveTag>();
    for (auto [e, pos] : player.each())
    {
        assert(batch.count < batch.buffer->size() / sizeof(OverviewMapVertex));

        auto& v = batch.mapped[batch.count];
        v.position = pos.value;
        v.color = glm::vec4{0, 1, 0, 1};

        batch.count++;
    }

    auto enemies = registry_->view<const ngn::Position, EnemyTag, ngn::ActiveTag>();
    for (auto [e, pos] : enemies.each())
    {
        assert(batch.count < batch.buffer->size() / sizeof(OverviewMapVertex));

        auto& v = batch.mapped[batch.count];
        v.position = pos.value;
        v.color = glm::vec4{1, 0, 0, 1};

        batch.count++;
    }
}

void OverviewMap::draw(ngn::CommandBuffer* commandBuffer)
{
    const auto frameIndex = renderer_->currentFrame();
    commandBuffer->bindPipeline(pipeline_);
    commandBuffer->bindDescriptorSet(pipeline_, pipeline_->descriptorSet(frameIndex));

    auto& batch = batches_[frameIndex];
    commandBuffer->bindVertexBuffer(batch.buffer);
    commandBuffer->draw(batch.count);

    batch.count = 0;
}
