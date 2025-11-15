// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Pipeline.hpp"

#include "Renderer.hpp"

namespace ngn {

PipelineConfig::PipelineConfig(Renderer* _renderer) :
    renderer{_renderer}
{
}

// *********************************************************************************************************************

Pipeline::Pipeline(const PipelineConfig& config) :
    renderer_{config.renderer},
    device_{renderer_->device()},
    bindPoint_{config.bindPoint}
{
    auto vertexShaderModule = createShaderModule(config.vertexShaderCode);
    auto fragmentShaderModule = createShaderModule(config.fragmentShaderCode);
    vk::ShaderModule geometryShaderModule;

    std::array<vk::PipelineShaderStageCreateInfo, 3> shaderStageCreateInfos{{
        {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = vertexShaderModule,
            .pName = "main",
        },
        {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = fragmentShaderModule,
            .pName = "main",
        },
    }};
    uint32_t shaderStageCreateInfoCount = 2;

    if (config.geometryShaderCode.data())
    {
        if (config.topology == vk::PrimitiveTopology::ePointList)
        {

        }

        geometryShaderModule = createShaderModule(config.geometryShaderCode);

        shaderStageCreateInfos[2] = {
            .stage = vk::ShaderStageFlagBits::eGeometry,
            .module = geometryShaderModule,
            .pName = "main",
        };

        shaderStageCreateInfoCount = 3;
    }

    // ****************************************************

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.setBindings(config.descriptorSetLayout);

    descriptorSetLayout_ = device_.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

    // ****************************************************

    std::array dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.setDynamicStates(dynamicStates);

    // ****************************************************

    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.setVertexBindingDescriptions(config.vertexBinding);
    vertexInputCreateInfo.setVertexAttributeDescriptions(config.vertexAttributes);

    // ****************************************************

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
        .topology = config.topology,
        .primitiveRestartEnable = false,
    };

    // ****************************************************

    vk::Viewport viewport{
        0.0f,
        0.0f,
        static_cast<float>(renderer_->swapChainExtent().width),
        static_cast<float>(renderer_->swapChainExtent().height),
        0.0f,
        1.0f,
    };

    vk::Rect2D scissor{
        {0, 0},
        renderer_->swapChainExtent(),
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.setViewports(viewport);
    viewportStateCreateInfo.setScissors(scissor);

    // ****************************************************

    vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo{
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    // ****************************************************

    vk::PipelineMultisampleStateCreateInfo multisamplingCreateInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = false,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false,
    };

    // ****************************************************

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = false,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA,
    };

    // ****************************************************

    if (config.blendEnabled)
    {
        colorBlendAttachment.blendEnable = true;
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendingCreateInfo{
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eCopy,
        .blendConstants = {},
    };
    colorBlendingCreateInfo.setAttachments(colorBlendAttachment);

    // ****************************************************

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.setSetLayouts(descriptorSetLayout_);

    pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutCreateInfo);

    // ****************************************************

    vk::GraphicsPipelineCreateInfo createInfo{
        .stageCount = shaderStageCreateInfoCount,
        .pStages = shaderStageCreateInfos.data(),
        .pVertexInputState = &vertexInputCreateInfo,
        .pInputAssemblyState = &inputAssemblyCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizerCreateInfo,
        .pMultisampleState = &multisamplingCreateInfo,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlendingCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = pipelineLayout_,
        .renderPass = renderer_->renderPass(),
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };

    const auto pipelines  = device_.createGraphicsPipelines({}, createInfo);
    graphicsPipeline_ = pipelines.value[0];

    // ****************************************************

    std::array<vk::DescriptorSetLayout, MaxFramesInFlight> descriptorSetLayouts;
    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        descriptorSetLayouts[i] = descriptorSetLayout_;
    };
    vk::DescriptorSetAllocateInfo descriptorSetAllocInfo{
        .descriptorPool = renderer_->descriptorPool(),
    };
    descriptorSetAllocInfo.setSetLayouts(descriptorSetLayouts);

    descriptorSets_ = renderer_->device().allocateDescriptorSets(descriptorSetAllocInfo);

    // ****************************************************

    device_.destroyShaderModule(fragmentShaderModule);
    device_.destroyShaderModule(vertexShaderModule);

    if (geometryShaderModule)
        device_.destroyShaderModule(geometryShaderModule);
}

Pipeline::~Pipeline()
{
    // create descriptorpool with VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    // device_.freeDescriptorSets(renderer_->descriptorPool(), descriptorSets_);
    device_.destroyPipeline(graphicsPipeline_);
    device_.destroyPipelineLayout(pipelineLayout_);
    device_.destroyDescriptorSetLayout(descriptorSetLayout_);
}

vk::ShaderModule Pipeline::createShaderModule(BufferView shaderCode)
{
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = shaderCode.size(),
        .pCode = reinterpret_cast<uint32_t*>(shaderCode.data()),
    };

    return device_.createShaderModule(createInfo);
}

void Pipeline::updateDescriptorSet(vk::DescriptorBufferInfo bufferInfo, uint32_t set, uint32_t binding, uint32_t arrayIndex)
{
    vk::WriteDescriptorSet write{
        .dstSet = descriptorSets_[set],
        .dstBinding = binding,
        .dstArrayElement = arrayIndex,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &bufferInfo,
    };
    renderer_->device().updateDescriptorSets(write, {});
}

void Pipeline::updateDescriptorSet(vk::DescriptorImageInfo imageInfo, uint32_t set, uint32_t binding, uint32_t arrayIndex)
{
    vk::WriteDescriptorSet write{
        .dstSet = descriptorSets_[set],
        .dstBinding = binding,
        .dstArrayElement = arrayIndex,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo,
    };
    renderer_->device().updateDescriptorSets(write, {});
}

} // namespace ngn
