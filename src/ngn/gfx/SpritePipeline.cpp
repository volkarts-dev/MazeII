// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "SpritePipeline.hpp"

#include "Assets.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Pipeline.hpp"
#include "Renderer.hpp"

namespace ngn {

SpritePipeline::SpritePipeline(Renderer* renderer) :
    renderer_{renderer}
{
    PipelineConfig config{renderer_};

    config.topology = vk::PrimitiveTopology::ePointList;

    config.vertexShaderCode = assets::shader_Sprite_vert_spv();
    config.geometryShaderCode = assets::shader_Sprite_geom_spv();
    config.fragmentShaderCode = assets::shader_Sprite_frag_spv();

    std::array descriptorSetLayout{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eGeometry,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 10,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        },
    };

    config.descriptorSetLayout = descriptorSetLayout;

    auto vertexDescription = SpriteVertex::description();
    config.vertexBinding = vertexDescription.first;
    config.vertexAttributes = vertexDescription.second;

    config.blendEnabled = true;

    pipeline_ = new Pipeline{config};
}

SpritePipeline::~SpritePipeline()
{
    delete pipeline_;
}

void SpritePipeline::updateDescriptorSet(vk::DescriptorBufferInfo bufferInfo, uint32_t set, uint32_t binding,
                                         uint32_t arrayIndex)
{
    pipeline_->updateDescriptorSet(bufferInfo, set, binding, arrayIndex);
}

void SpritePipeline::updateDescriptorSet(vk::DescriptorImageInfo imageInfo, uint32_t set, uint32_t binding,
                                         uint32_t arrayIndex)
{
    pipeline_->updateDescriptorSet(imageInfo, set, binding, arrayIndex);
}

vk::DescriptorSet SpritePipeline::descriptorSet(uint32_t frame)
{
    return pipeline_->descriptorSet(frame);
}

} // namespace
