// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

template<typename T, std::size_t _Size, std::unsigned_integral SizeT = uint32_t>
class Array
{
    static_assert(_Size <= std::numeric_limits<SizeT>::max(), "Size type cannot hold capacity");

public:
    constexpr static SizeT Size = static_cast<SizeT>(_Size);

    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;
    using const_iterator = const_pointer;

public:
    pointer data() { return data_; }
    const_pointer data() const { return data_; }

    SizeT capaciy() const { return Size; }
    SizeT size() const { return Size; }
    bool empty() const { return Size == 0; }

    iterator begin() { return data_; }
    const_iterator begin() const { return data_; }
    const_iterator cbegin() const { return data_; }

    iterator end() { return data_ + Size; }
    const_iterator end() const { return data_ + Size; }
    const_iterator cend() const { return data_ + Size; }

    reference operator[](SizeT index) { return at(index); }
    const_reference operator[](SizeT index) const { return at(index); }

    reference at(SizeT index)
    {
        assert(index < Size);
        return data_[index];
    }

    const_reference at(SizeT index) const
    {
        assert(index < Size);
        return data_[index];
    }

    T data_[Size];
};

} // namespace ngn
