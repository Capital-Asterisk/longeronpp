/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#include <longeron/containers/bit_view.hpp>

#include <gtest/gtest.h>

#include <cstdint>

#include <array>
#include <random>
#include <vector>

/**
 * @brief Generate random integers in ascending order up to a certain maximum
 */
static std::vector<int> random_ascending(int seed, size_t maximum)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, 10);
    std::vector<int> testSet;

    for (size_t i = 0; i < maximum; i ++)
    {
        if (dist(gen) == 1)
        {
            testSet.push_back(i);
        }
    }

    return testSet;
}

template <typename INT_T>
void set_reset_test()
{
    std::vector<INT_T> data{0x04, 0x00, 0x00, 0x00};

    lgrn::BitView bits = lgrn::bit_view(data);

    // reset single bit
    ASSERT_TRUE(bits.test(2));
    bits.reset(2);
    ASSERT_FALSE(bits.test(2));

    // set single bit
    ASSERT_FALSE(bits.test(18));
    bits.set(18);
    ASSERT_TRUE(bits.test(18));

    // set all bits
    bits.set();
    ASSERT_EQ(data[0], INT_T(~INT_T(0)));
    ASSERT_EQ(data[3], INT_T(~INT_T(0)));

    // clear all bits
    bits.reset();
    ASSERT_EQ(data[0], 0);
    ASSERT_EQ(data[3], 0);
}

// Basic setting and clearing of bits
TEST(BitView, SetAndReset)
{
    set_reset_test<uint8_t>();
    set_reset_test<uint16_t>();
    set_reset_test<uint32_t>();
    set_reset_test<uint64_t>();
}

template <typename INT_T>
void positions_test(std::size_t bitSize)
{
    std::vector<INT_T> data;
    data.resize(bitSize * sizeof(INT_T) / 8 + 1, 0);

    lgrn::BitView bits = lgrn::bit_view(data);

    std::vector<int> const positions = random_ascending(42, bitSize);

    // Make each position correspond with a 1 bit

    for (int const pos : positions)
    {
        bits.set(pos);
    }

    {
        auto posIt = positions.begin();

        for (std::size_t pos : bits.ones())
        {
            ASSERT_EQ(pos, *posIt);
            std::advance(posIt, 1);
        }
    }

    bits.set();

    // Make each position correspond with a 0 bit

    for (int const pos : positions)
    {
        bits.reset(pos);
    }

    {
        auto posIt = positions.begin();

        for (std::size_t pos : bits.zeros())
        {
            ASSERT_EQ(pos, *posIt);
            std::advance(posIt, 1);
        }
    }
}

// Test iterating ones bits and zeros bits
TEST(BitView, IteratePositons)
{
    std::size_t const sc_bitSize = 133700;

    positions_test<uint8_t>(sc_bitSize);
    positions_test<uint16_t>(sc_bitSize);
    positions_test<uint32_t>(sc_bitSize);
    positions_test<uint64_t>(sc_bitSize);
}
