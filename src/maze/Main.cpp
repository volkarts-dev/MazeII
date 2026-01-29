// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "MazeDelegate.hpp"

int main() {
    MazeDelegate delegate{};

    ngn::Application app{&delegate};

    app.exec();

    return 0;
}
