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

std::pair<bool, Duration<double> > Timer::elapsed(Duration<double> secs)
{
    const auto now = clock_.now();
    const auto end = start_ + secs;
    const auto diff = now - start_;
    const auto e = now > end;
    if (e)
        start_ = now;
    return std::make_pair(e, diff);
}

} // namespace ngn
