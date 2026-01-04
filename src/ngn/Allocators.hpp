// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Macros.hpp"
#include "Types.hpp"

namespace ngn {

class MemoryArena
{
private:
    using Byte = uint8_t;
    using Ptr = Byte*;

public:
    MemoryArena(std::size_t size);
    ~MemoryArena();

    std::size_t capacity() const { return capacity_; }
    std::size_t allocated() const { return top_; }

    std::size_t statAllocatedCount() const { return statAllocatedCount_; }
    std::size_t statAllocatedSize() const { return statAllocatedSize_; }
    std::size_t statDeallocatedCount() const { return statDeallocatedCount_; }
    std::size_t statDeallocatedSize() const { return statDeallocatedSize_; }

    void* allocate(std::size_t size, std::size_t alignment);
    void deallocate(void* ptr, std::size_t size);
    void* reallocate(void* ptr, std::size_t size, std::size_t alignment);

    void reset();

private:
    std::size_t align(std::size_t ptr, std::size_t alignment);

private:
    Byte* data_;
    std::size_t capacity_;
    std::size_t top_;
    std::size_t lastTop_;
    std::size_t lastAlloc_;

    std::size_t statAllocatedCount_;
    std::size_t statAllocatedSize_;
    std::size_t statDeallocatedCount_;
    std::size_t statDeallocatedSize_;

    NGN_DISABLE_COPY_MOVE(MemoryArena)
};

template<typename T>
class LinearAllocator
{
public:
    using value_type = T;

    explicit LinearAllocator(MemoryArena* arena) noexcept :
        arena_{arena}
    {
    }

    template <typename U>
    LinearAllocator(const LinearAllocator<U>& other) noexcept :
        arena_{other.arena_}
    {
    }

    template <typename U>
    friend class LinearAllocator;

    T* allocate(std::size_t count)
    {
        return static_cast<T*>(arena_->allocate(count * sizeof(T), alignof(T)));
    }

    void deallocate(T* ptr, std::size_t count) noexcept
    {
        arena_->deallocate(ptr, count * sizeof(T));
    }

    bool operator==(const LinearAllocator& other) const noexcept {
        return arena_ == other.arena_;
    }

    bool operator!=(const LinearAllocator& other) const noexcept {
        return !(*this == other);
    }

private:
    MemoryArena* arena_;
};

} // namespace ngn
