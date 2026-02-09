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
    std::pair<bool, float> elapsed(bool reset = false);
    std::pair<bool, float> elapsed(float secs);

private:
    float time_;
};

} // namespace ngn
