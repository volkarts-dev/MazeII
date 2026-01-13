// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Timer.hpp"


namespace ngn {

Timer::Timer()
{
    reset();
}

void Timer::reset()
{
    start_ = clock_.now();
}

std::pair<bool, Duration<double> > Timer::elapsed(bool reset)
{
    const auto now = clock_.now();
    const auto diff = now - start_;
    if (reset)
        start_ = now;
    return std::make_pair(false, diff);
}

std::pair<bool, Duration<double> > Timer::elapsed(Duration<double> secs)
{
    const auto now = clock_.now();
    const auto diff = now - start_;
    const auto end = start_ + secs;
    const auto reset = now > end;
    if (reset)
        start_ = now;
    return std::make_pair(reset, diff);
}

} // namespace ngn
