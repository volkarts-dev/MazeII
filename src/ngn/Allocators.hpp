// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"

namespace ngn {

class LinearAllocator
{
private:
    using Byte = uint8_t;
    using Ptr = Byte*;

public:
    LinearAllocator(std::size_t size);
    ~LinearAllocator();

    void* allocate(std::size_t size, std::size_t alignment);
    void deallocate(void* ptr);
    void* reallocate(void* ptr, std::size_t size, std::size_t alignment);

    void Reset();

private:
    std::size_t align(std::size_t ptr, std::size_t alignment);

private:
    Byte* data_;
    std::size_t capacity_;
    std::size_t top_;
    std::size_t lastTop_;
    std::size_t lastAlloc_;

    NGN_DISABLE_COPY_MOVE(LinearAllocator)
};

} // namespace ngn
