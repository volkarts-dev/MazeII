// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

template<typename T, std::size_t Capacity>
class StaticVector
{
public:
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;
    using const_iterator = const_pointer;

public:
    pointer data() { return data_; }
    const_pointer data() const { return data_; }

    std::size_t capaciy() const { return Capacity; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    iterator begin() { return data_; }
    const_iterator begin() const { return data_; }
    const_iterator cbegin() const { return data_; }

    iterator end() { return data_ + size_; }
    const_iterator end() const { return data_ + size_; }
    const_iterator cend() const { return data_ + size_; }

    reference operator[](std::size_t index) { return at(index); }
    const_reference operator[](std::size_t index) const { return at(index); }

    reference front()
    {
        assert(size_);
        return data_[0];
    }

    const_reference front() const
    {
        assert(size_);
        return data_[0];
    }

    reference back()
    {
        assert(size_);
        return data_[size_ - 1];
    }
    const_reference back() const
    {
        assert(size_);
        return data_[size_ - 1];
    }

    reference at(std::size_t index)
    {
        assert(index < size_);
        return data_[index];
    }

    const_reference at(std::size_t index) const
    {
        assert(index < size_);
        return data_[index];
    }

    template<typename... Args>
    reference emplace_back(Args... args)
    {
        assert(size_ < Capacity);
        auto ptr = std::construct_at(data_ + size_, std::forward<Args>(args)...);
        ++size_;
        return *ptr;
    }

    void pop_back()
    {
        assert(size_);
        --size_;
        std::destroy_at(data_ + size_);
    }

private:
    T data_[Capacity];
    std::size_t size_{};
};

} // namespace ngn
