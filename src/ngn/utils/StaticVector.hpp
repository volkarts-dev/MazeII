// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

template<typename T, std::size_t Capacity, std::unsigned_integral SizeT = uint32_t>
class StaticVector
{
    static_assert(Capacity <= std::numeric_limits<SizeT>::max(), "Size type cannot hold capacity");

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

    reference at(SizeT index)
    {
        assert(index < size_);
        return data_[index];
    }

    const_reference at(SizeT index) const
    {
        assert(index < size_);
        return data_[index];
    }

    template<typename... Args>
    reference emplace_back(Args&&... args)
    {
        if (size_ >= Capacity)
            throw std::out_of_range("StaticVector capacity exceeded");
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
    SizeT size_{};
};

} // namespace ngn
