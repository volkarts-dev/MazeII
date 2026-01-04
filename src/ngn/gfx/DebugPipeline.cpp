// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "DebugPipeline.hpp"

#include "Assets.hpp"
#include "Pipeline.hpp"
#include "Renderer.hpp"

namespace ngn {

DebugPipeline::DebugPipeline(Renderer* renderer, Mode mode) :
    renderer_{renderer},
    mode_{mode}
{
    PipelineConfig config{renderer_};

    switch (mode)
    {
        using enum Mode;

        case Line:
            config.topology = vk::PrimitiveTopology::eLineList;
            break;

        case Fill:
            config.topology = vk::PrimitiveTopology::eTriangleList;
            break;
    }


    config.vertexShaderCode = assets::shader_Debug_vert_spv();
    config.fragmentShaderCode = assets::shader_Debug_frag_spv();

    std::array descriptorSetLayout{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        },
    };

    config.descriptorSetLayout = descriptorSetLayout;

    auto vertexDescription = DebugVertex::description();
    config.vertexBinding = vertexDescription.first;
    config.vertexAttributes = vertexDescription.second;

    config.blendEnabled = true;

    pipeline_ = new Pipeline{config};
}

DebugPipeline::~DebugPipeline()
{
    delete pipeline_;
}

void DebugPipeline::updateDescriptorSet(vk::DescriptorBufferInfo bufferInfo, uint32_t set, uint32_t binding,
                                        uint32_t arrayIndex)
{
    pipeline_->updateDescriptorSet(bufferInfo, set, binding, arrayIndex);
}

vk::DescriptorSet DebugPipeline::descriptorSet(uint32_t frame)
{
    return pipeline_->descriptorSet(frame);
}

} // namespace ngn
