// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include <string_view>

namespace ngn {

namespace  {

using namespace std::literals;

constexpr std::array Units = { "B"sv, "KB"sv, "MB"sv, "GB"sv, "TB"sv, "PB"sv, "EB"sv };

} // namespace

void humanReadableBytes(std::size_t bytes, std::size_t& outNum, std::string_view& outUnit)
{

    outNum = bytes;
    std::size_t unitIndex = 0;

    while (outNum >= 1024 && unitIndex < Units.size() - 1)
    {
        outNum /= 1024;
        ++unitIndex;
    }

    outUnit = Units[unitIndex];
}

} // namespace
