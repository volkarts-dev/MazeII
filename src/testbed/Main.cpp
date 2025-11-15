#include "TestBedDelegate.hpp"

int main() {
    TestBedDelegate delegate{};

    ngn::Application app{&delegate};

    app.exec();

    return 0;
}
