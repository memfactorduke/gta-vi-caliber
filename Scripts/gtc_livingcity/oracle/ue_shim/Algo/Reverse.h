// Out-of-tree compile shim for UE's Algo::Reverse. NOT a UE header.
#pragma once

#include <algorithm>

namespace Algo
{
    template <typename RangeType>
    void Reverse(RangeType& Range) { std::reverse(Range.begin(), Range.end()); }
}
