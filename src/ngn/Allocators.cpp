// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Allocators.hpp"

namespace ngn {

MemoryArena::MemoryArena(std::size_t size) :
    data_{new Byte[size]},
    capacity_{size},
    top_{},
    lastTop_{},
    lastAlloc_{},
    statAllocatedCount_{},
    statAllocatedSize_{},
    statDeallocatedCount_{},
    statDeallocatedSize_{}
{
}

MemoryArena::~MemoryArena()
{
    delete[] data_;
}

void* MemoryArena::allocate(std::size_t size, std::size_t alignment)
{
    const auto start = align(top_, alignment);
    
    // Check for overflow before addition
    if (start > capacity_ - size)
        throw std::runtime_error("Out of memory or integer overflow in allocation");
    
    const auto end = start + size;

    if (end > capacity_)
        throw std::runtime_error("Out of memory.");

    lastTop_ = top_;
    lastAlloc_ = size;

    top_ = end;

    //log::debug("lastTop: {}, lastAlloc: {}, top: {}, size: {}, ptr: {}", lastTop_, lastAlloc_, top_, size, reinterpret_cast<void*>(data_ + start));
    statAllocatedCount_++;
    statAllocatedSize_ += size;

    return data_ + start;
}

void MemoryArena::deallocate(void* ptr, std::size_t size)
{
    // deallocate last block, ignore other pointers
    if (data_ + (top_ - lastAlloc_) == ptr)
    {
        top_ = lastTop_;
    }

    statDeallocatedCount_++;
    statDeallocatedSize_ += size;
}

void* MemoryArena::reallocate(void* ptr, std::size_t size, std::size_t alignment)
{
    deallocate(ptr, lastAlloc_);
    return allocate(size, alignment);
}

void MemoryArena::reset()
{
    top_ = 0;

    statAllocatedCount_ = 0;
    statAllocatedSize_ = 0;
    statDeallocatedCount_ = 0;
    statDeallocatedSize_ = 0;
}

std::size_t MemoryArena::align(std::size_t pos, std::size_t alignment)
{
    return (pos + (alignment - 1)) & ~(alignment - 1);
}

} // namespace ngn
