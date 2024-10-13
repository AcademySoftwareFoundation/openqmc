// SPDX-License-Identifier: Apache-2.0
// Copyright Contributors to the OpenQMC Project.

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <algorithm>

void rotateAndFlip(int n, int& x, int& y, int rx, int ry)
{
    if (ry == 0)
    {
        if (rx != 0)
        {
            x = n - 1 - x;
            y = n - 1 - y;
        }

        std::swap(x, y);
    }
}

std::uint32_t hilbertIndex(int n, int x, int y)
{
    assert(n > 0);
    assert((n & (n - 1)) == 0); // n should be a power of 2.
    assert(x >= 0);
    assert(x < n);
    assert(y >= 0);
    assert(y < n);

    auto index = std::uint32_t();

    for (auto s = n / 2; s > 0; s /= 2)
    {
        auto rx = static_cast<int>((x & s) != 0);
        auto ry = static_cast<int>((y & s) != 0);

        index += s * s * ((3 * rx) ^ ry);

        rotateAndFlip(s, x, y, rx, ry);
    }

    return index;
}

void printHilbertIndices(int n)
{
    for (auto y = 0; y < n; ++y)
    {
        for (auto x = 0; x < n; ++x)
        {
            std::printf("%u,\n", hilbertIndex(n, x, y));
        }
    }
}

void runUnitTests()
{
    assert(hilbertIndex(2, 0, 0) == 0);
    assert(hilbertIndex(2, 1, 0) == 3);
    assert(hilbertIndex(2, 1, 1) == 2);
    assert(hilbertIndex(2, 0, 1) == 1);

    assert(hilbertIndex(16, 0, 0) == 0);
    assert(hilbertIndex(16, 15, 0) == 255);
}

int main()
{
    runUnitTests();
    constexpr auto n = 256;

    printHilbertIndices(n);

    return 0;
}
