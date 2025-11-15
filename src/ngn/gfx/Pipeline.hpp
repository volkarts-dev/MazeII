// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include <vulkan/vulkan.hpp>

namespace ngn {

class Renderer;

class PipelineConfig
{
public:
    PipelineConfig(Renderer* _renderer);

    Renderer* const renderer;

    vk::PipelineBindPoint bindPoint{vk::PipelineBindPoint::eGraphics};
    BufferView vertexShaderCode{};
    BufferView geometryShaderCode{};
    BufferView fragmentShaderCode{};
    std::span<vk::DescriptorSetLayoutBinding> descriptorSetLayout;
    vk::VertexInputBindingDescription vertexBinding;
    std::span<vk::VertexInputAttributeDescription> vertexAttributes;
    vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
    bool blendEnabled{};
};

class Pipeline
{
public:
    Pipeline(const PipelineConfig& config);
    ~Pipeline();

    const vk::Pipeline& handle() const { return graphicsPipeline_; }
    const vk::PipelineLayout& layout() const { return pipelineLayout_; }

    vk::PipelineBindPoint bindPoint() const { return bindPoint_; }
    vk::DescriptorSet descriptorSet(uint32_t frame) { return descriptorSets_[frame]; }

    void updateDescriptorSet(vk::DescriptorBufferInfo bufferInfo, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0);
    void updateDescriptorSet(vk::DescriptorImageInfo imageInfo, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0);

private:
    vk::ShaderModule createShaderModule(BufferView shaderCode);

private:
    Renderer* renderer_;
    vk::Device device_;

    vk::PipelineBindPoint bindPoint_;
    vk::DescriptorSetLayout descriptorSetLayout_;
    vk::PipelineLayout pipelineLayout_;
    vk::Pipeline graphicsPipeline_;

    std::vector<vk::DescriptorSet> descriptorSets_;

    NGN_DISABLE_COPY_MOVE(Pipeline)
};

} // namespace ngn
