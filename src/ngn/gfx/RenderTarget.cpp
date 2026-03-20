// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "RenderTarget.hpp"

#include "Renderer.hpp"
#include "gfx/Image.hpp"

namespace ngn {

RenderTarget::RenderTarget(const RenderTargetCreateInfo& createInfo) :
    renderer_{createInfo.renderer},
    clearColor_{createInfo.clearColor}
{
    vk::AttachmentDescription colorAttachment{
        .format = createInfo.imagesFormat,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = createInfo.finalImagesLayout,
    };

    vk::AttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
    };
    subpass.setColorAttachments(colorAttachmentRef);

    vk::RenderPassCreateInfo renderPassCreateInfo{
    };
    renderPassCreateInfo.setAttachments(colorAttachment);
    renderPassCreateInfo.setSubpasses(subpass);
    renderPassCreateInfo.setDependencies(createInfo.dependencies);

    renderPass_ = renderer_->device().createRenderPass(renderPassCreateInfo);

    framebuffers_.reserve(createInfo.colorAttachments.size());

    createFramebuffers(createInfo.imagesSize, createInfo.colorAttachments);
}

RenderTarget::~RenderTarget()
{
    destroyFramebuffers();

    renderer_->device().destroyRenderPass(renderPass_);
}

void RenderTarget::recreateFramebuffers(vk::Extent2D imagesSize, std::span<ImageView*> colorAttachments)
{
    destroyFramebuffers();
    framebuffers_.clear();
    createFramebuffers(imagesSize, colorAttachments);
}

void RenderTarget::createFramebuffers(vk::Extent2D imagesSize, std::span<ImageView*> colorAttachments)
{
    for (const auto& imageView : colorAttachments)
    {
        vk::FramebufferCreateInfo frameBufferCreateInfo{
            .renderPass = renderPass_,
            .width = imagesSize.width,
            .height = imagesSize.height,
            .layers = 1,
        };
        frameBufferCreateInfo.setAttachments(imageView->handle());

        framebuffers_.push_back(renderer_->device().createFramebuffer(frameBufferCreateInfo));
    }

    framebuffersSize_ = imagesSize;
}

void RenderTarget::destroyFramebuffers()
{
    for (const auto& framebuffer: framebuffers_)
    {
        renderer_->device().destroyFramebuffer(framebuffer);
    }
}

} // namespace ngn
