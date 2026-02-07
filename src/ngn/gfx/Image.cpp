// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Image.hpp"

#include "Buffer.hpp"
#include "Renderer.hpp"
#include "StbImage.hpp"

namespace ngn {

ImageLoader ImageLoader::createFromBitmap(Renderer* renderer, uint32_t width, uint32_t height, const BufferView buffer)
{
    ImageLoader loader;
    loader.renderer_ = renderer;
    loader.width_ = width;
    loader.height_ = height;

    // Check for integer overflow before multiplication
    if (width > 0 && height > 0) {
        const uint64_t imageSize64 = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4;
        if (imageSize64 > std::numeric_limits<vk::DeviceSize>::max())
            throw std::runtime_error("Image dimensions too large - would cause integer overflow");
    }

    const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(loader.width_) * 
                                     static_cast<vk::DeviceSize>(loader.height_) * 4;
    if (imageSize != buffer.size())
    {
        throw std::runtime_error("width * height != buffer_size");
    }

    BufferConfig config{renderer, vk::BufferUsageFlagBits::eTransferSrc, imageSize};
    config.hostVisible = true;
    loader.buffer_ = std::make_unique<Buffer>(config);

    auto range = loader.buffer_->map();
    std::memcpy(range.data(), buffer.data(), imageSize);
    loader.buffer_->unmap();

    return loader;
}

ImageLoader ImageLoader::loadFromBuffer(Renderer* renderer, const BufferView buffer)
{
    int texWidth{};
    int texHeight{};
    int texChannels{};
    stbi_uc* pixels = stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()),
                                            &texWidth, &texHeight, &texChannels,
                                            STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image.");
    }

    // Validate dimensions before casting to unsigned
    if (texWidth <= 0 || texHeight <= 0 || texChannels <= 0)
    {
        stbi_image_free(pixels);
        throw std::runtime_error("Invalid image dimensions returned from stbi_load");
    }

    // Enforce reasonable maximum dimensions to prevent overflow
    if (texWidth > 65536 || texHeight > 65536)
    {
        stbi_image_free(pixels);
        throw std::runtime_error("Image dimensions exceed maximum safe size (65536x65536)");
    }

    const auto width = static_cast<uint32_t>(texWidth);
    const auto height = static_cast<uint32_t>(texHeight);
    const auto channels = static_cast<uint32_t>(texChannels);

    // Check for integer overflow in buffer size calculation
    const uint64_t bufferSize64 = static_cast<uint64_t>(width) * 
                                  static_cast<uint64_t>(height) * 
                                  static_cast<uint64_t>(channels);
    if (bufferSize64 > std::numeric_limits<std::size_t>::max())
    {
        stbi_image_free(pixels);
        throw std::runtime_error("Image buffer size would overflow");
    }

    auto loader = createFromBitmap(
                renderer,
                width, height,
                BufferView{pixels, static_cast<std::size_t>(bufferSize64)}
                );

    stbi_image_free(pixels);

    return loader;
}

ImageLoader::ImageLoader()
{
}

ImageLoader::~ImageLoader()
{
}

// *********************************************************************************************************************

Image::Image(const ImageLoader& loader) :
    renderer_{loader.renderer_},
    format_{vk::Format::eR8G8B8A8Srgb}
{
    vk::ImageCreateInfo createInfo{
        .imageType = vk::ImageType::e2D,
        .format = format_,
        .extent = {
            .width = loader.width_,
            .height = loader.height_,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    image_ = renderer_->device().createImage(createInfo);

    auto memRequirements = renderer_->device().getImageMemoryRequirements(image_);

    auto memType = renderer_->findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::MemoryAllocateInfo allocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = memType,
    };

    memory_ = renderer_->device().allocateMemory(allocateInfo);

    renderer_->device().bindImageMemory(image_, memory_, 0);

    renderer_->transitionImageLayout(this, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    renderer_->copyBuffer(loader.buffer_.get(), this, {}, {loader.width_, loader.height_});
    renderer_->transitionImageLayout(this, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

Image::~Image()
{
    renderer_->device().destroyImage(image_);
    renderer_->device().freeMemory(memory_);
}

// *********************************************************************************************************************

ImageView::ImageView(const Image* image) :
    ImageView{image->renderer(), image->format(), image->handle()}
{
}

ImageView::ImageView(Renderer* renderer, vk::Format format, vk::Image image) :
    renderer_{renderer},
    format_{format}
{
    vk::ImageViewCreateInfo createInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format_,
        .components = {
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    imageView_ = renderer_->device().createImageView(createInfo);
}

ImageView::~ImageView()
{
    renderer_->device().destroyImageView(imageView_);

}

// *********************************************************************************************************************

Sampler::Sampler(Renderer* renderer, vk::Filter filter, vk::SamplerAddressMode mode, bool unnormalizedCoords) :
    renderer_{renderer}
{
    vk::SamplerCreateInfo createInfo{
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = unnormalizedCoords ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear,
        .addressModeU = mode,
        .addressModeV = mode,
        .addressModeW = mode,
        .mipLodBias = 0.0f,
        .anisotropyEnable = !unnormalizedCoords,
        .maxAnisotropy = renderer_->physicalDeviceProperties().limits.maxSamplerAnisotropy,
        .compareEnable = false,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = unnormalizedCoords,
    };

    sampler_ = renderer_->device().createSampler(createInfo);
}

Sampler::~Sampler()
{
    renderer_->device().destroySampler(sampler_);
}

} // namespace ngn
