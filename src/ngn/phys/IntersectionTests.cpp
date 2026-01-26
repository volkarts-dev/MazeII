// Copyright 2026, Daniel Volk <mail@volkarts.com>
// SPDX-License-Identifier: <LICENSE>

#include "IntersectionTests.hpp"

#include "phys/Shapes.hpp"

namespace ngn {

namespace {

} // namespace

bool intersects(const AABB& lhs, const AABB& rhs)
{
    return
        (lhs.bottomRight.x >= rhs.topLeft.x) &
        (lhs.bottomRight.y >= rhs.topLeft.y) &
        (rhs.bottomRight.x >= lhs.topLeft.x) &
        (rhs.bottomRight.y >= lhs.topLeft.y);
}

bool intersects(const Line& lhs, const AABB& rhs)
{
    AABB test = rhs;

    const auto invD = 1.0f / (lhs.end - lhs.start);
    if (invD.x < 0)
        std::swap(test.topLeft.x, test.bottomRight.x);
    if (invD.y < 0)
        std::swap(test.topLeft.y, test.bottomRight.y);

    const auto near = (test.topLeft - lhs.start) * invD;
    const auto far = (test.bottomRight - lhs.start) * invD;

    if (near.x > far.x && near.y > far.y)
        return false;

    const auto nearT = near.x > near.y ? near.x : near.y;
    const auto farT = far.x < far.y ? far.x : far.y;

    if (nearT > 1.0f || farT < 0.0f)
        return false;

    return true;
}

} // namespace ngn
