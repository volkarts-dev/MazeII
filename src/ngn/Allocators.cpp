// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Allocators.hpp"

namespace ngn {

LinearAllocator::LinearAllocator(std::size_t size) :
    data_{new Byte[size]},
    capacity_{size},
    top_{},
    lastTop_{},
    lastAlloc_{}
{
}

LinearAllocator::~LinearAllocator()
{
    delete[] data_;
}

void* LinearAllocator::allocate(std::size_t size, std::size_t alignment)
{
    const auto start = align(top_, alignment);
    const auto end = start + size;

    if (end > capacity_)
        throw std::runtime_error("Out of memory.");

    lastTop_ = top_;
    lastAlloc_ = size;

    top_ = end;

    return data_ + start;
}

void LinearAllocator::deallocate(void* ptr)
{
    NGN_UNUSED(ptr);
    // Do nothing per design
}

void* LinearAllocator::reallocate(void* ptr, std::size_t size, std::size_t alignment)
{
    if (data_ + (top_ - lastAlloc_) == ptr)
    {
        top_ = lastTop_;
    }

    return allocate(size, alignment);
}

void LinearAllocator::Reset()
{
    top_ = 0;
}

std::size_t LinearAllocator::align(std::size_t pos, std::size_t alignment)
{
    return (pos + (alignment - 1)) & ~(alignment - 1);
}

} // namespace ngn
