/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#include <longeron/containers/intarray_multimap.hpp>

#include <gtest/gtest.h>

using lgrn::IntArrayMultiMap;

TEST(IntArrayMultiMap, Basic)
{
    lgrn::IntArrayMultiMap<unsigned int, float> multimap(30, 10);

    multimap.emplace(0, {1.0f, 2.0f});
    multimap.emplace(1, {3.0f, 4.0f});
    multimap.emplace(2, {5.0f, 6.0f});

    EXPECT_TRUE(multimap.contains(0));
    EXPECT_TRUE(multimap.contains(2));
    EXPECT_FALSE(multimap.contains(3));

    EXPECT_EQ(multimap[0][0], 1.0f);
    EXPECT_EQ(multimap[2][1], 6.0f);

    multimap.erase(1);

    EXPECT_FALSE(multimap.contains(1));

    multimap.pack();

    EXPECT_EQ(multimap[0][0], 1.0f);
    EXPECT_EQ(multimap[2][1], 6.0f);
}
