// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

namespace ngn {

class ImageView;
class Renderer;

class RenderTargetCreateInfo
{
public:
    Renderer* renderer;
    vk::Extent2D imagesSize;
    vk::Format imagesFormat;
    vk::ImageLayout finalImagesLayout;
    std::span<ImageView*> colorAttachments;
    vk::ArrayProxy<vk::SubpassDependency> dependencies;
    glm::vec4 clearColor{0, 0, 0, 1};
};

class RenderTarget
{
public:
    RenderTarget(const RenderTargetCreateInfo& createInfo);
    ~RenderTarget();

    const vk::RenderPass& renderPass() const { return renderPass_; }
    const vk::Framebuffer& framebuffer(uint32_t imageIndex) const { return framebuffers_[imageIndex]; }
    vk::Extent2D framebuffersSize() const { return framebuffersSize_; }
    const glm::vec4& clearColor() const { return clearColor_; }

    void recreateFramebuffers(vk::Extent2D imagesSize, std::span<ImageView*> colorAttachments);

private:
    void createFramebuffers(vk::Extent2D imagesSize, std::span<ImageView*> colorAttachments);
    void destroyFramebuffers();

private:
    Renderer* renderer_;
    vk::RenderPass renderPass_;
    std::vector<vk::Framebuffer> framebuffers_;
    vk::Extent2D framebuffersSize_;
    glm::vec4 clearColor_;

    NGN_DISABLE_COPY_MOVE(RenderTarget)
};

} // namespace ngn
