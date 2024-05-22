/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#include <longeron/id_management/id_set_stl.hpp>

#include <gtest/gtest.h>


enum class Id : uint64_t { };

// Touch every member function. You know how a set works, right?
TEST(IdSet, BasicUse)
{
    lgrn::IdSetStl<Id> set;

    set.resize(40);

    ASSERT_GE(set.capacity(), 40);
    ASSERT_TRUE(set.empty());

    {
        auto const [pos, insertSuccess] = set.insert(Id{0});
        ASSERT_TRUE(set.contains(Id{0}));
        EXPECT_TRUE(insertSuccess);
    }
    {
        auto const [pos, insertSuccess] = set.insert(Id{0});
        ASSERT_TRUE(set.contains(Id{0}));
        EXPECT_FALSE(insertSuccess);
    }

    set.emplace(Id{2});
    set.insert({Id{6}, Id{9}, Id{8}});

    auto const currentIds = {Id{0}, Id{2}, Id{6}, Id{8}, Id{9}};

    ASSERT_EQ(set.size(), 5);
    ASSERT_TRUE(std::equal(currentIds.begin(), currentIds.end(), set.begin()));
}
