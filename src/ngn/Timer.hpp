// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Types.hpp"

namespace ngn {

class Timer
{
public:
    Timer();

    void reset();
    std::pair<bool, Duration<double>> elapsed(Duration<double> secs = {});

private:
    Clock clock_;
    Timepoint  start_;
};

} // namespace ngn
