/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#include <longeron/containers/intarray_multimap.hpp>

#include <gtest/gtest.h>

using lgrn::IntArrayMultiMap;


TEST(IntArrayMultiMap, Basic)
{
    lgrn::IntArrayMultiMap<int, float> multimap(20);
    multimap.resize_ids(5);
    auto in = {2.0f, 3.0f, 4.0f};
    float* data = multimap.emplace(1, in.begin(), in.end());

    auto inB = {9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f, 9.0f};
    float* dataB = multimap.emplace(2, inB.begin(), inB.end());

    multimap.erase(1);
}
