// Copyright 2025, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: MIT

#include "Shapes.hpp"

namespace ngn {

Shape::Shape(Circle&& c) :
    type{Type::Circle},
    circle{std::move(c)}
{
}

Shape::Shape(Line&& l) :
    type{Type::Line},
    line{std::move(l)}
{
}

Shape::Shape(Capsule&& c) :
    type{Type::Capsule},
    capsule{std::move(c)}
{
}

} // namespace ngn
