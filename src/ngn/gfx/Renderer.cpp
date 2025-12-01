// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Renderer.hpp"

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Image.hpp"
#include "Logging.hpp"
#include "Pipeline.hpp"
#include "Types.hpp"
#include <GLFW/glfw3.h>
#include <map>

namespace ngn {

namespace {

#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)

constexpr std::array ValidationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

constexpr std::array ValidationExtensions = {
    "VK_EXT_debug_utils",
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/)
{
    log::error("Validation layer: [{}] [{}] {}",
               vk::to_string(messageType), vk::to_string(messageSeverity), pCallbackData->pMessage);

    return VK_FALSE;
}

vk::DebugUtilsMessengerCreateInfoEXT makeDebugCreateInfo()
{
    return {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning /*|
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose*/,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
         .pfnUserCallback = debugMessengerCallback,
    };
}

PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

#endif

constexpr std::array DeviceExtensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

} // namespace

Renderer::Renderer(GLFWwindow* window) :
    window_{window},
    currentFrame_{},
    framebufferResized_{false}
{
    createInstance();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();
    createCommandPools();
    createCommandBuffers();
    createDescriptorPool();
}

void Renderer::createInstance()
{
    vk::ApplicationInfo appInfo{
        .pApplicationName = "MazeII",
        .applicationVersion = VK_MAKE_VERSION(2, 0, 0),
        .pEngineName = "MazeII",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion12,

    };

    // collect required layers
    std::vector<const char*> layers;

#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    std::copy(ValidationLayers.begin(), ValidationLayers.end(), std::back_inserter(layers));
#endif

    // collect required extensions
    std::vector<const char*> extensions;

#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    std::copy(ValidationExtensions.begin(), ValidationExtensions.end(), std::back_inserter(extensions));
#endif

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++)
    {
      extensions.push_back(glfwExtensions[i]);
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
    };
    createInfo.setPEnabledLayerNames(layers);
    createInfo.setPEnabledExtensionNames(extensions);

#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    const auto earlyDebugCreateInfo = makeDebugCreateInfo();
    createInfo.setPNext(&earlyDebugCreateInfo);
#endif

    instance_ = vk::createInstance(createInfo);

#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    pfnVkCreateDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance_.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    if (!pfnVkCreateDebugUtilsMessengerEXT)
    {
      throw std::runtime_error("GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function.");
    }

    pfnVkDestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(instance_.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if (!pfnVkDestroyDebugUtilsMessengerEXT)
    {
      throw std::runtime_error("GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.");
    }

    const auto debugCreateInfo = makeDebugCreateInfo();
    debugMessenger_ = instance_.createDebugUtilsMessengerEXT(debugCreateInfo);
#endif
}

void Renderer::createSurface()
{
    VkSurfaceKHR s{};
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &s) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
    surface_ = s;
}

void Renderer::selectPhysicalDevice()
{
    const auto devices = instance_.enumeratePhysicalDevices();

    std::multimap<int, vk::PhysicalDevice> candidates;
    for (const auto& device : devices)
    {
        candidates.insert({calcDeviceScore(device), device});
    }

    if (candidates.rbegin()->first > 0)
        physicalDevice_ = candidates.rbegin()->second;

    if (!physicalDevice_)
        throw std::runtime_error("Failed to find a suitable GPU");

    physicalDeviceProperties_ = physicalDevice_.getProperties();
    maxMssaSampleCount_ = maxUsableSampleCount(physicalDeviceProperties_);

    log::info("Choosen GPU: {}", std::string_view{physicalDevice_.getProperties().deviceName.data()});
}

void Renderer::createLogicalDevice()
{
    const auto queueFamilies = queryQueueFamilies(physicalDevice_);
    const auto uniqueIndices = queueFamilies.uniqueIndices();

    float queuePriorities = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const auto& index : uniqueIndices)
    {
        queueCreateInfos.push_back({
            .queueFamilyIndex = index,
            .queueCount = 1,
            .pQueuePriorities = &queuePriorities,
        });
    }

    vk::PhysicalDeviceFeatures features{
        .geometryShader = true,
        .samplerAnisotropy = true,
    };

    vk::DeviceCreateInfo createInfo{
        .pEnabledFeatures = &features,
    };
    createInfo.setQueueCreateInfos(queueCreateInfos);
    createInfo.setPEnabledExtensionNames(DeviceExtensions);
#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    createInfo.setPEnabledLayerNames(ValidationLayers);
#endif

    device_ = physicalDevice_.createDevice(createInfo);

    graphicsQueue_ = device_.getQueue(queueFamilies.graphicsIndex.value(), 0);
    presentQueue_ = device_.getQueue(queueFamilies.presentIndex.value(), 0);
    transferQueue_ = device_.getQueue(queueFamilies.transferIndex.value(), 0);
}

void Renderer::createSwapChain()
{
    const auto surfaceDetails = queryDeviceSurfaceDetails(physicalDevice_);

    const auto surfaceFormat = chooseSwapSurfaceFormat(surfaceDetails.formats);
    const auto presentMode = chooseSwapPresentMode(surfaceDetails.presentModes);
    const auto extent = chooseSwapExtent(surfaceDetails.capabilities);

    auto imageCount = surfaceDetails.capabilities.minImageCount + 1;

    if (surfaceDetails.capabilities.maxImageCount > 0 && imageCount > surfaceDetails.capabilities.maxImageCount)
        imageCount = surfaceDetails.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo{
        .surface = surface_,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform =surfaceDetails.capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true,
    };

    const auto queueFamilies = queryQueueFamilies(physicalDevice_);

    if (queueFamilies.graphicsIndex != queueFamilies.presentIndex)
    {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        std::array queueFamilyIndices{queueFamilies.graphicsIndex.value(), queueFamilies.presentIndex.value()};
        createInfo.setQueueFamilyIndices(queueFamilyIndices);
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapChain_ = device_.createSwapchainKHR(createInfo);

    swapChainImages_ = device_.getSwapchainImagesKHR(swapChain_);
    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;
}

void Renderer::createImageViews()
{
    swapChainImageViews_.reserve(swapChainImages_.size());

    for (const auto& image : swapChainImages_)
    {
        swapChainImageViews_.push_back(new ImageView{this, swapChainImageFormat_, image});
    }
}

void Renderer::createRenderPass()
{
    vk::AttachmentDescription colorAttachment{
        .format = swapChainImageFormat_,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };

    vk::AttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
    };
    subpass.setColorAttachments(colorAttachmentRef);

    vk::SubpassDependency dependency{
        .srcSubpass = vk::SubpassExternal,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
    };

    vk::RenderPassCreateInfo createInfo{
    };
    createInfo.setAttachments(colorAttachment);
    createInfo.setSubpasses(subpass);
    createInfo.setDependencies(dependency);

    renderPass_ = device_.createRenderPass(createInfo);
}

void Renderer::createFramebuffers()
{
    swapChainFramebuffers_.reserve(swapChainImageViews_.size());

    for ( auto& imageView : swapChainImageViews_)
    {
        vk::FramebufferCreateInfo createInfo{
            .renderPass = renderPass_,
            .width = swapChainExtent_.width,
            .height = swapChainExtent_.height,
            .layers = 1,
        };
        createInfo.setAttachments(imageView->handle());

        swapChainFramebuffers_.push_back(device_.createFramebuffer(createInfo));
    }
}

void Renderer::createSyncObjects()
{
    vk::SemaphoreCreateInfo semaphorCreateInfo{};

    vk::FenceCreateInfo fenceCreateInfo{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    };

    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        imageAvailableSemaphores_[i] = device_.createSemaphore(semaphorCreateInfo);
        renderFinishedSemaphores_[i] = device_.createSemaphore(semaphorCreateInfo);
        inFlightFences_[i] = device_.createFence(fenceCreateInfo);
    }
}

void Renderer::createCommandPools()
{
    const auto queueFamilies = queryQueueFamilies(physicalDevice_);

    vk::CommandPoolCreateInfo createInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilies.graphicsIndex.value(),
    };

    commandPool_ = device_.createCommandPool(createInfo);

    createInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
    createInfo.queueFamilyIndex = queueFamilies.transferIndex.value();

    immediateCommandPool_ = device_.createCommandPool(createInfo);
}

void Renderer::createCommandBuffers()
{
    CommandBufferConfig config{this};

    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        commandBuffers_[i] = new CommandBuffer{config};
    }
}

void Renderer::createDescriptorPool()
{
    std::array poolSizes{
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = MaxFramesInFlight,
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = MaxFramesInFlight * MaxSpritePipelineTextures,
        },
    };

    vk::DescriptorPoolCreateInfo createInfo{
        .maxSets = MaxFramesInFlight,
    };
    createInfo.setPoolSizes(poolSizes);

    descriptorPool_ = device_.createDescriptorPool(createInfo);
}

// *********************************************************************************************************************

Renderer::~Renderer()
{
    device_.destroyDescriptorPool(descriptorPool_);

    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        delete commandBuffers_[i];
    }

    device_.destroyCommandPool(immediateCommandPool_);
    device_.destroyCommandPool(commandPool_);

    for (uint32_t i = 0; i < MaxFramesInFlight; i++)
    {
        device_.destroyFence(inFlightFences_[i]);
        device_.destroySemaphore(renderFinishedSemaphores_[i]);
        device_.destroySemaphore(imageAvailableSemaphores_[i]);
    }

    device_.destroyRenderPass(renderPass_);

    destroySwapChain();

    device_.destroy();

#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)
    instance_.destroyDebugUtilsMessengerEXT(debugMessenger_);
#endif

    instance_.destroySurfaceKHR(surface_);

    instance_.destroy();
}

// *********************************************************************************************************************

uint32_t Renderer::startFrame()
{
    const auto result = device_.waitForFences(inFlightFences_[currentFrame_], true, UINT64_MAX);
    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return InvalidIndex;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device_.resetFences(inFlightFences_[currentFrame_]);

    const auto imageIndex = device_.acquireNextImageKHR(swapChain_, UINT64_MAX, imageAvailableSemaphores_[currentFrame_]);

    return imageIndex.value;
}

void Renderer::endFrame(uint32_t imageIndex)
{
    vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphores(renderFinishedSemaphores_[currentFrame_]);
    presentInfo.setSwapchains(swapChain_);
    presentInfo.setImageIndices(imageIndex);

    const auto result = presentQueue_.presentKHR(presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized_)
    {
        framebufferResized_ = false;
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame_++;
    if (currentFrame_ >= MaxFramesInFlight)
        currentFrame_ = 0;
}

void Renderer::submit(CommandBuffer* commandBuffer)
{
    const auto cb = commandBuffer->handle();
    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo{};
    submitInfo.setWaitSemaphores(imageAvailableSemaphores_[currentFrame_]);
    submitInfo.setWaitDstStageMask(waitDstStageMask);
    submitInfo.setCommandBuffers(cb);
    submitInfo.setSignalSemaphores(renderFinishedSemaphores_[currentFrame_]);

    graphicsQueue_.submit(submitInfo, inFlightFences_[currentFrame_]);
}

void Renderer::waitForDevice()
{
    device_.waitIdle();
}

uint32_t Renderer::findMemoryType(uint32_t memoryTypes, vk::MemoryPropertyFlags memoryFlags)
{
    const auto memProperties = physicalDevice_.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (memoryTypes & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & memoryFlags) == memoryFlags)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

void Renderer::copyBuffer(Buffer* src, Buffer* dest, std::size_t size, std::size_t srcOff, std::size_t dstOff)
{
    auto commandBuffer = beginImmediateCommands();

    vk::BufferCopy copyRegion{
        .srcOffset = srcOff,
        .dstOffset = dstOff,
        .size = size,
    };
    commandBuffer.copyBuffer(src->handle(), dest->handle(), copyRegion);

    endImmediateCommands(commandBuffer);
}

void Renderer::copyBuffer(Buffer* src, Image* dest, vk::Offset2D offset, vk::Extent2D size)
{
    auto commandBuffer = beginImmediateCommands();

    vk::BufferImageCopy region{
        .bufferOffset = 0, // TODO support buffer offset
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {offset.x, offset.y, 0},
        .imageExtent = {size.width, size.height, 1},
    };
    commandBuffer.copyBufferToImage(src->handle(), dest->handle(), vk::ImageLayout::eTransferDstOptimal, region);

    endImmediateCommands(commandBuffer);
}

void Renderer::transitionImageLayout(Image* image, vk::ImageLayout srcLayout, vk::ImageLayout destLayout)
{
    auto commandBuffer = beginImmediateCommands();

    vk::ImageMemoryBarrier barrier{
        .srcAccessMask = {},
        .dstAccessMask = {},
        .oldLayout = srcLayout,
        .newLayout = destLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image->handle(),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags destStage;

    if (srcLayout == vk::ImageLayout::eUndefined && destLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (srcLayout == vk::ImageLayout::eTransferDstOptimal && destLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        destStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition.");
    }

    commandBuffer.pipelineBarrier(srcStage, destStage, {}, {}, {}, barrier);

    endImmediateCommands(commandBuffer);
}

// *********************************************************************************************************************

void Renderer::destroySwapChain()
{
    for (const auto& framebuffer: swapChainFramebuffers_)
    {
        device_.destroyFramebuffer(framebuffer);
    }
    swapChainFramebuffers_.clear();

    for (auto& imageView : swapChainImageViews_)
    {
        delete imageView;
    }
    swapChainImageViews_.clear();

    device_.destroySwapchainKHR(swapChain_);
}

void Renderer::recreateSwapChain()
{
    waitForDevice();

    auto framebufferSize = getFramebufferSize();
    while (framebufferSize.width == 0 || framebufferSize.height == 0)
    {
        glfwWaitEvents();
        framebufferSize = getFramebufferSize();
    }

    destroySwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
}

// *********************************************************************************************************************

uint32_t Renderer::calcDeviceScore(vk::PhysicalDevice device) const
{
    auto indices = queryQueueFamilies(device);
    if (!indices.isComplete())
        return 0;

    auto deviceSurfaceDetails = queryDeviceSurfaceDetails(device);
    if (deviceSurfaceDetails.formats.empty() || deviceSurfaceDetails.presentModes.empty())
        return 0;

    if (!checkDeviceExtensionSupport(device))
        return 0;

    vk::PhysicalDeviceFeatures features = device.getFeatures();
    if (!features.samplerAnisotropy || !features.geometryShader)
        return 0;

    auto properties = device.getProperties();

    uint32_t score = 0;

    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;
    else if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
        score += 800;

    score += properties.limits.maxImageDimension2D / 32;

    return score;
}

DeviceQueueFamilies Renderer::queryQueueFamilies(vk::PhysicalDevice device) const
{
    auto queueFamilyProperties = device.getQueueFamilyProperties();

    DeviceQueueFamilies indices{};
    uint32_t i{0};
    for (const auto& f : queueFamilyProperties)
    {
        if (f.queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphicsIndex = i;

        if (device.getSurfaceSupportKHR(i, surface_))
            indices.presentIndex = i;

        if (f.queueFlags & vk::QueueFlagBits::eTransfer)
            indices.transferIndex = i;

        i++;
    }

    return indices;
}

DeviceSurfaceDetails Renderer::queryDeviceSurfaceDetails(vk::PhysicalDevice device) const
{
    DeviceSurfaceDetails details{
        .capabilities = device.getSurfaceCapabilitiesKHR(surface_),
        .formats = device.getSurfaceFormatsKHR(surface_),
        .presentModes = device.getSurfacePresentModesKHR(surface_),
    };
    return details;
}

bool Renderer::checkDeviceExtensionSupport(vk::PhysicalDevice device) const
{
    std::vector<vk::ExtensionProperties> deviceExtensions = device.enumerateDeviceExtensionProperties(nullptr);

    std::set<std::string> requiredExtensions{DeviceExtensions.begin(), DeviceExtensions.end()};

    for (const auto& extension : deviceExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

vk::SampleCountFlagBits Renderer::maxUsableSampleCount(vk::PhysicalDeviceProperties properties) const
{
    vk::SampleCountFlags counts =
            properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e64)
        return vk::SampleCountFlagBits::e64;
    if (counts & vk::SampleCountFlagBits::e32)
        return vk::SampleCountFlagBits::e32;
    if (counts & vk::SampleCountFlagBits::e16)
        return vk::SampleCountFlagBits::e16;
    if (counts & vk::SampleCountFlagBits::e8)
        return vk::SampleCountFlagBits::e8;
    if (counts & vk::SampleCountFlagBits::e4)
        return vk::SampleCountFlagBits::e4;
    if (counts & vk::SampleCountFlagBits::e2)
        return vk::SampleCountFlagBits::e2;

    return vk::SampleCountFlagBits::e1;
}
vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) const
{
    for (const auto& format : availableFormats)
    {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return format;
    }
    return availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) const
{
    // prefer mailbox over fifo when available
    for (const auto& mode : availablePresentModes)
    {
        if (mode == vk::PresentModeKHR::eMailbox)
            return mode;
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        auto actualExtent = getFramebufferSize();

        actualExtent.width =
                std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height =
                std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

vk::Extent2D Renderer::getFramebufferSize() const
{
    int width{}, height{};
    glfwGetFramebufferSize(window_, &width, &height);

    return vk::Extent2D{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
}

vk::CommandBuffer Renderer::beginImmediateCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = immediateCommandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    auto commandBuffer = device_.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void Renderer::endImmediateCommands(vk::CommandBuffer commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.setCommandBuffers(commandBuffer);
    graphicsQueue_.submit(submitInfo);

    graphicsQueue_.waitIdle();

    device_.freeCommandBuffers(immediateCommandPool_, commandBuffer);
}

} // namespace ngn

#if defined(NGN_ENABLE_GRAPHICS_DEBUG_LAYER)

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
        VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
{
    return ngn::pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
        VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks const * pAllocator)
{
    return ngn::pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

#endif
