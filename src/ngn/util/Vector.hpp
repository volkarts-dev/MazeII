// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#pragma once

#include "Macros.hpp"
namespace ngn {

template<typename T, typename Alloc, typename SizeType = uint32_t, bool AvoidCopy = true>
class Vector
{
public:

public:
    Vector(Alloc* alloc) noexcept :
        alloc_{alloc}
    {
    }

    ~Vector()
    {
        if (data_)
            alloc_->deallocate(data_);
    }

    Vector(Vector&& other) noexcept :
        alloc_{}
    {
        swap(other);
    }

    Vector& operator=(Vector&& other)
    {
        swap(other);
        return *this;
    }

    void swap(Vector& other) noexcept
    {
        using std::swap;

        std::swap(alloc_, other.alloc_);
        std::swap(data_, other.data_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
    }

    void pushBack(T&& value)
    {
        if (size_ >= capacity_)
            grow();
        data_[size_] = std::move(value);
        size_++;
    }

    void pushBack(const T& value)
    {
        if (size_ >= capacity_)
            grow();
        data_[size_] = value;
        size_++;
    }

    SizeType capacity() const { return capacity_; }
    SizeType size() const { return size_; }

    T& operator[](SizeType index) { return data_[index]; }
    const T& operator[](SizeType index) const { return data_[index]; }

    T* begin() { return data_; }
    const T* begin() const { return data_; }
    const T* Cbegin() const { return data_; }
    T* end() { return data_ + size_; }
    const T* end() const { return data_ + size_; }
    const T* cend() const { return data_ + size_; }

private:
    void grow()
    {
        auto newCapacity = std::max(SizeType{1}, capacity_ * 2);
        auto* newPtr = reinterpret_cast<T*>(alloc_->reallocate(data_, newCapacity, alignof(T)));

        if constexpr (AvoidCopy)
        {
            if (data_ && newPtr != data_)
                throw std::runtime_error("Vector can only grow inplace.");
        }
        else
        {
            throw std::runtime_error("Copying Vector data is not supported yet.");
        }

        data_ = newPtr;
        capacity_ = newCapacity;
    }

private:
    Alloc* alloc_;

    T* data_{};
    SizeType capacity_{};
    SizeType size_{};

    NGN_DISABLE_COPY(Vector)
};

} // namespace
