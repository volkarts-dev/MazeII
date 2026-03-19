// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

namespace ngn {

class Timer
{
public:
    Timer();

    void update(float deltaTime);

    void restart(bool hot = false);
    float elapsedTime() { return time_; }
    std::pair<bool, float> isElapsed(float secs, bool reset = true);

private:
    float time_;
};

} // namespace ngn
