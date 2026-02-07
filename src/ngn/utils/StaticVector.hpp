// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

template<typename T, std::size_t Capacity, std::unsigned_integral SizeT = uint32_t>
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

    SizeT capaciy() const { return Capacity; }
    SizeT size() const { return size_; }
    bool empty() const { return size_ == 0; }

    iterator begin() { return data_; }
    const_iterator begin() const { return data_; }
    const_iterator cbegin() const { return data_; }

    iterator end() { return data_ + size_; }
    const_iterator end() const { return data_ + size_; }
    const_iterator cend() const { return data_ + size_; }

    reference operator[](SizeT index) { return at(index); }
    const_reference operator[](SizeT index) const { return at(index); }

    reference front()
    {
        if (size_ == 0)
            throw std::out_of_range("StaticVector::front() called on empty container");
        return data_[0];
    }

    const_reference front() const
    {
        if (size_ == 0)
            throw std::out_of_range("StaticVector::front() called on empty container");
        return data_[0];
    }

    reference back()
    {
        if (size_ == 0)
            throw std::out_of_range("StaticVector::back() called on empty container");
        return data_[size_ - 1];
    }
    const_reference back() const
    {
        if (size_ == 0)
            throw std::out_of_range("StaticVector::back() called on empty container");
        return data_[size_ - 1];
    }

    reference at(SizeT index)
    {
        if (index >= size_)
            throw std::out_of_range("StaticVector index out of bounds");
        return data_[index];
    }

    const_reference at(SizeT index) const
    {
        if (index >= size_)
            throw std::out_of_range("StaticVector index out of bounds");
        return data_[index];
    }

    template<typename... Args>
    reference emplace_back(Args... args)
    {
        if (size_ >= Capacity)
            throw std::out_of_range("StaticVector capacity exceeded");
        auto ptr = std::construct_at(data_ + size_, std::forward<Args>(args)...);
        ++size_;
        return *ptr;
    }

    void pop_back()
    {
        if (size_ == 0)
            throw std::out_of_range("StaticVector::pop_back() called on empty container");
        --size_;
        std::destroy_at(data_ + size_);
    }

private:
    T data_[Capacity];
    SizeT size_{};
};

} // namespace ngn
