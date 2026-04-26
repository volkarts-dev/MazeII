// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include <vulkan/vulkan.hpp>
#include <set>

struct GLFWwindow;

namespace ngn {

class Buffer;
class CommandBuffer;
class Image;
class ImageView;
class Pipeline;
class RenderTarget;

class DeviceQueueFamilies
{
public:
    bool isComplete()
    {
        return graphicsIndex.has_value() &&
                presentIndex.has_value() &&
                transferIndex.has_value();
    }

    auto uniqueIndices() const
    {
        return std::set{
            graphicsIndex.value(),
            presentIndex.value(),
            transferIndex.value(),
        };
    }

    auto indices() const
    {
        return std::array{
            graphicsIndex.value(),
            presentIndex.value(),
            transferIndex.value()
        };
    }

public:
    std::optional<uint32_t> graphicsIndex;
    std::optional<uint32_t> presentIndex;
    std::optional<uint32_t> transferIndex;
};

class DeviceSurfaceDetails
{
public:
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class DescriptorSetAllocationInfo
{
public:
    std::span<vk::DescriptorSetLayoutBinding> bindings;
    vk::DescriptorSetLayout layout;
    uint32_t count;
};

class Renderer
{
public:
    Renderer(GLFWwindow* window);
    ~Renderer();

    const vk::PhysicalDeviceProperties& physicalDeviceProperties() const { return physicalDeviceProperties_; }
    const vk::Device& device() const { return device_; }
    const vk::Extent2D& swapChainExtent() const { return swapChainExtent_; }
    RenderTarget* renderTarget() const { return renderTarget_; }
    const vk::CommandPool& commandPool() const { return commandPool_; }
    void triggerFramebufferResized() { framebufferResized_ = true; }
    uint32_t currentFrame() const { return currentFrame_; }
    CommandBuffer* currentCommandBuffer() { return commandBuffers_[currentFrame_]; }

    std::vector<vk::DescriptorSet> allocateDescriptorSets(const DescriptorSetAllocationInfo& allocInfo);

    uint32_t startFrame();
    void endFrame(uint32_t imageIndex);
    void submit(CommandBuffer* commandBuffer);

    void waitForDevice();
    uint32_t findMemoryType(uint32_t memoryTypes, vk::MemoryPropertyFlags memoryFlags);
    void copyBuffer(Buffer* src, Buffer* dest, std::size_t size, std::size_t srcOff = 0, std::size_t dstOff = 0);
    void copyBuffer(Buffer* src, Image* dest, vk::Extent2D size, std::size_t srcOff, vk::Offset2D dstOff);
    void transitionImageLayout(Image* image, vk::ImageLayout srcLayout, vk::ImageLayout destLayout);

private:
    void createInstance();
    void createSurface();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderTarget();
    void createSyncObjects();
    void createCommandPools();
    void createCommandBuffers();

    void destroySwapChain();
    void recreateSwapChain();

    vk::DescriptorPool createDescriptorPool(const DescriptorSetAllocationInfo& allocInfo);
    uint32_t calcDeviceScore(vk::PhysicalDevice device) const;
    DeviceQueueFamilies queryQueueFamilies(vk::PhysicalDevice device) const;
    DeviceSurfaceDetails queryDeviceSurfaceDetails(vk::PhysicalDevice device) const;
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device) const;
    vk::SampleCountFlagBits maxUsableSampleCount(vk::PhysicalDeviceProperties properties) const;
    vk::Extent2D getFramebufferSize() const;

    vk::CommandBuffer beginImmediateCommands();
    void endImmediateCommands(vk::CommandBuffer commandBuffer);

private:
    GLFWwindow* window_;
    vk::Instance instance_;
#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    vk::DebugUtilsMessengerEXT debugMessenger_;
#endif
    vk::SurfaceKHR surface_;
    vk::PhysicalDevice physicalDevice_;
    vk::PhysicalDeviceProperties physicalDeviceProperties_;
    vk::Device device_;
    vk::SampleCountFlags maxMssaSampleCount_;
    vk::Queue graphicsQueue_;
    vk::Queue presentQueue_;
    vk::Queue transferQueue_;
    vk::SwapchainKHR swapChain_;
    std::vector<vk::Image> swapChainImages_;
    vk::SurfaceFormatKHR swapChainImageFormat_;
    vk::Extent2D swapChainExtent_;
    std::vector<ImageView*> swapChainImageViews_;
    RenderTarget* renderTarget_;
    std::array<vk::Semaphore, MaxFramesInFlight> imageAvailableSemaphores_;
    std::array<vk::Semaphore, MaxFramesInFlight> renderFinishedSemaphores_;
    std::array<vk::Fence, MaxFramesInFlight> inFlightFences_;
    vk::CommandPool commandPool_;
    vk::CommandPool immediateCommandPool_;
    std::array<CommandBuffer*, MaxFramesInFlight> commandBuffers_;

    std::unordered_map<vk::DescriptorType, uint32_t> descriptorPoolSizes_;
    uint32_t descriptorMaxSets_;
    std::vector<vk::DescriptorPool> descriptorPools_;

    uint32_t currentFrame_;
    bool framebufferResized_;

    NGN_DISABLE_COPY_MOVE(Renderer)
};

} // namespace ngn
