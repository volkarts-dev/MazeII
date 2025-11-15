// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Types.hpp"
#include <vulkan/vulkan.hpp>
#include <memory>

namespace ngn {

class Buffer;
class Renderer;

class ImageLoader
{
public:
    static ImageLoader createFromBitmap(Renderer* renderer, uint32_t width, uint32_t height, BufferView buffer);
    static ImageLoader loadFromBuffer(Renderer* renderer, BufferView buffer);

    ~ImageLoader();

    NGN_DEFAULT_MOVE(ImageLoader)

private:
    ImageLoader();

    Renderer* renderer_;
    std::unique_ptr<Buffer> buffer_;
    uint32_t width_;
    uint32_t height_;

    NGN_DISABLE_COPY(ImageLoader)

    friend class Image;
};

// *********************************************************************************************************************

class Image
{
public:
    Image(const ImageLoader& loader);
    ~Image();

    Renderer* renderer() const { return renderer_; }
    const vk::Image& handle() const { return image_; }
    vk::Format format() const { return format_; }

private:
    Renderer* renderer_;
    vk::Format format_;
    vk::Image image_;
    vk::DeviceMemory memory_;

    NGN_DISABLE_COPY_MOVE(Image)
};

// *********************************************************************************************************************

class ImageView
{
public:
    ImageView(Image* image);
    ImageView(Renderer* renderer, vk::Format format, vk::Image image);
    ~ImageView();

    Renderer* renderer() const { return renderer_; }
    const vk::ImageView& handle() const { return imageView_; }
    vk::Format format() const { return format_; }

private:
    Renderer* renderer_;
    vk::Format format_;
    vk::ImageView imageView_;

    NGN_DISABLE_COPY_MOVE(ImageView)
};

// *********************************************************************************************************************

class Sampler
{
public:
    Sampler(Renderer* renderer, vk::Filter filter, vk::SamplerAddressMode mode);
    ~Sampler();

    const vk::Sampler& handle() const { return sampler_; }

private:
    Renderer* renderer_;
    vk::Sampler sampler_;

    NGN_DISABLE_COPY_MOVE(Sampler)
};

} // namespace ngn
