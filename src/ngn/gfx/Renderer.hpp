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

class Renderer
{
public:
    Renderer(GLFWwindow* window);
    ~Renderer();

    const vk::PhysicalDeviceProperties& physicalDeviceProperties() const { return physicalDeviceProperties_; }
    const vk::Device& device() const { return device_; }
    const vk::Extent2D& swapChainExtent() const { return swapChainExtent_; }
    const vk::RenderPass& renderPass() const { return renderPass_; }
    const vk::CommandPool& commandPool() const { return commandPool_; }
    const vk::Framebuffer& swapChainFramebuffer(uint32_t imageIndex) const { return swapChainFramebuffers_[imageIndex]; }
    void triggerFramebufferResized() { framebufferResized_ = true; }
    uint32_t currentFrame() const { return currentFrame_; }
    CommandBuffer* currentCommandBuffer() { return commandBuffers_[currentFrame_]; }
    const vk::DescriptorPool& descriptorPool() const { return descriptorPool_; }

    uint32_t startFrame();
    void endFrame(uint32_t imageIndex);
    void submit(CommandBuffer* commandBuffer);

    void waitForDevice();
    uint32_t findMemoryType(uint32_t memoryTypes, vk::MemoryPropertyFlags memoryFlags);
    void copyBuffer(Buffer* src, Buffer* dest, std::size_t size, std::size_t srcOff = 0, std::size_t dstOff = 0);
    void copyBuffer(Buffer* src, Image* dest, vk::Offset2D offset, vk::Extent2D size);
    void transitionImageLayout(Image* image, vk::ImageLayout srcLayout, vk::ImageLayout destLayout);

private:
    void createInstance();
    void createSurface();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();
    void createCommandPools();
    void createCommandBuffers();
    void createDescriptorPool();

    void destroySwapChain();
    void recreateSwapChain();

    uint32_t calcDeviceScore(vk::PhysicalDevice device) const;
    DeviceQueueFamilies queryQueueFamilies(vk::PhysicalDevice device) const;
    DeviceSurfaceDetails queryDeviceSurfaceDetails(vk::PhysicalDevice device) const;
    bool checkDeviceExtensionSupport(vk::PhysicalDevice device) const;
    vk::SampleCountFlagBits maxUsableSampleCount(vk::PhysicalDeviceProperties properties) const;
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const;
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const;
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    vk::Extent2D getFramebufferSize() const;

    vk::CommandBuffer beginImmediateCommands();
    void endImmediateCommands(vk::CommandBuffer commandBuffer);

private:
    GLFWwindow* window_;
    vk::Instance instance_;
#if !defined(NDEBUG)
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
    vk::Format swapChainImageFormat_;
    vk::Extent2D swapChainExtent_;
    std::vector<ImageView*> swapChainImageViews_;
    vk::RenderPass renderPass_;
    std::vector<vk::Framebuffer> swapChainFramebuffers_;
    std::array<vk::Semaphore, MaxFramesInFlight> imageAvailableSemaphores_;
    std::array<vk::Semaphore, MaxFramesInFlight> renderFinishedSemaphores_;
    std::array<vk::Fence, MaxFramesInFlight> inFlightFences_;
    vk::CommandPool commandPool_;
    vk::CommandPool immediateCommandPool_;
    std::array<CommandBuffer*, MaxFramesInFlight> commandBuffers_;
    vk::DescriptorPool descriptorPool_;

    uint32_t currentFrame_;
    bool framebufferResized_;

    NGN_DISABLE_COPY_MOVE(Renderer)
};

} // namespace ngn
