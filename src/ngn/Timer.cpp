// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Timer.hpp"


namespace ngn {

Timer::Timer()
{
    restart();
}

void Timer::update(float deltaTime)
{
    time_ += deltaTime;
}

void Timer::restart(bool hot)
{
    time_ = hot ? std::numeric_limits<float>::max() : 0.0f;
}

std::pair<bool, float> Timer::elapsed(bool reset)
{
    const auto time = time_;
    if (reset)
        time_ = 0.0f;
    return std::make_pair(false, time);
}

std::pair<bool, float> Timer::elapsed(float secs)
{
    const auto time = time_;
    const auto reset = time >= secs;
    if (reset)
        time_ = 0.0f;
    return std::make_pair(reset, time);
}

} // namespace ngn
