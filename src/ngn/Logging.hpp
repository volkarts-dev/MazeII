// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <spdlog/spdlog.h>

namespace ngn {

namespace log {

namespace level = spdlog::level;

using spdlog::set_level;

using spdlog::critical;
using spdlog::error;
using spdlog::warn;
using spdlog::info;
using spdlog::debug;
using spdlog::trace;

} // namespace log

void humanReadableBytes(std::size_t bytes, std::size_t& outNum, std::string_view& outUnit);
void humanReadableBytes(std::size_t bytes, double& outNum, std::string_view& outUnit);

struct Bytes
{
    std::size_t value;
};

} // namespace ngn

template<>
struct fmt::formatter<ngn::Bytes>
{
    constexpr auto parse(fmt::format_parse_context& ctx) const
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const ngn::Bytes& b, FormatContext& ctx) const
    {
        std::size_t num{};
        std::string_view unit{};
        ngn::humanReadableBytes(b.value, num, unit);
        return fmt::format_to(ctx.out(), "{} {}", num, unit);
    }
};
