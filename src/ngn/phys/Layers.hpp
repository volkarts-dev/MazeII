// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

enum class Layers : uint32_t
{
    L0  = 0x0000'0001, L1  = 0x0000'0002, L2  = 0x0000'0004, L3  = 0x0000'0008,
    L4  = 0x0000'0010, L5  = 0x0000'0020, L6  = 0x0000'0040, L7  = 0x0000'0080,
    L8  = 0x0000'0100, L9 = 0x0000'0200, L10 = 0x0000'0400, L11 = 0x0000'0800,
    L12 = 0x0000'1000, L13 = 0x0000'2000, L14 = 0x0000'4000, L15 = 0x0000'8000,
    L16 = 0x0001'0000, L17 = 0x0002'0000, L18 = 0x0004'0000, L19 = 0x0008'0000,
    L20 = 0x0010'0000, L21 = 0x0020'0000, L22 = 0x0040'0000, L23 = 0x0080'0000,
    L24 = 0x0100'0000, L25 = 0x0200'0000, L26 = 0x0400'0000, L27 = 0x0800'0000,
    L28 = 0x1000'0000, L29 = 0x2000'0000, L30 = 0x4000'0000, L31 = 0x8000'0000,

    B0 = 0x0000'000F, B1 = 0x0000'00F0, B2 = 0x0000'0F00, B3 = 0x0000'F000,
    B4 = 0x000F'0000, B5 = 0x00F0'0000, B6 = 0x0F00'0000, B7 = 0xF000'0000,

    All = 0xFFFF'FFFF,
};

inline constexpr Layers operator|(Layers lhs, Layers rhs)
{
    return static_cast<Layers>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline constexpr Layers operator&(Layers lhs, Layers rhs)
{
    return static_cast<Layers>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline constexpr bool toBool(Layers layer)
{
    return static_cast<uint32_t>(layer) != 0;
}

} // namespace
