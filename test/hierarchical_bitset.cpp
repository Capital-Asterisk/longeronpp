/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#include <longeron/containers/hierarchical_bitset.hpp>

#include <gtest/gtest.h>

#include <array>
#include <random>
#include <vector>

using lgrn::HierarchicalBitset;

/**
 * @brief Generate random integers in ascending order up to a certain maximum
 */
static std::vector<int> random_ascending(int seed, size_t maximum)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, 1);
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

// Test count trailing zeros function. This relies on compiler intrinsics.
TEST(HierarchicalBitset, CountTrailingZeros)
{
    using lgrn::ctz;

    EXPECT_EQ(  0, ctz(uint64_t(1)) );
    EXPECT_EQ( 20, ctz(uint64_t(1) << 20) );
    EXPECT_EQ( 63, ctz(uint64_t(1) << 63) );
    EXPECT_EQ(  2, ctz(0b0010'1100) );
    EXPECT_EQ(  4, ctz(0b0101'0000) );
}

// Test case where the container size is not aligned with its block size
TEST(HierarchicalBitset, BasicUnaligned)
{
    // with a size of 129, only 1 bit is used in the last block
    HierarchicalBitset bitset(129);

    bitset.set(0);
    bitset.set(42);
    bitset.set(128);

    EXPECT_TRUE( bitset.test(0) );
    EXPECT_TRUE( bitset.test(42) );
    EXPECT_TRUE( bitset.test(128) );
    EXPECT_EQ( 3, bitset.count() );

    bitset.reset(0);
    bitset.reset(128);

    EXPECT_FALSE( bitset.test(0) );
    EXPECT_TRUE( bitset.test(42) );
    EXPECT_FALSE( bitset.test(128) );
    EXPECT_EQ( bitset.count(), 1 );

    // Try taking 11 bits, but there's only 1 left (42)
    std::array<int, 11> toTake;
    toTake.fill(-1); // make sure garbage values don't ruin the test
    int const remainder = bitset.take(toTake.begin(), 11);

    EXPECT_EQ( 10, remainder );
    EXPECT_EQ( 42, toTake[0] );
    EXPECT_EQ( 0, bitset.count() );
}

// Test case where the container size is exactly aligned with its block size
TEST(HierarchicalBitset, BasicAligned)
{
    HierarchicalBitset bitset(128);

    bitset.set(0);
    bitset.set(42);
    bitset.set(127);

    EXPECT_TRUE( bitset.test(0) );
    EXPECT_TRUE( bitset.test(42) );
    EXPECT_TRUE( bitset.test(127) );
    EXPECT_EQ( 3, bitset.count() );

    bitset.reset(0);
    bitset.reset(127);

    EXPECT_FALSE( bitset.test(0) );
    EXPECT_TRUE( bitset.test(42) );
    EXPECT_FALSE( bitset.test(127) );
    EXPECT_EQ( 1, bitset.count() );

    // Try taking 11 bits, but there's only 1 left (42)
    std::array<int, 11> toTake;
    toTake.fill(-1); // make sure garbage values don't ruin the test
    int const remainder = bitset.take(toTake.begin(), 11);

    EXPECT_EQ( 10, remainder );
    EXPECT_EQ( 42, toTake[0] );
    EXPECT_EQ( 0, bitset.count() );
}

// Test setting random bits, and taking them all
TEST(HierarchicalBitset, TakeRandomSet)
{
    constexpr size_t const sc_max = 13370;
    constexpr size_t const sc_seed = 420;

    std::vector<int> const testSet = random_ascending(sc_seed, sc_max);

    HierarchicalBitset bitset(sc_max);

    for (int i : testSet)
    {
        bitset.set(i);
    }

    EXPECT_EQ( bitset.count(), testSet.size() );

    std::vector<int> results(testSet.size());

    int const remainder = bitset.take(results.begin(), testSet.size() + 12);

    EXPECT_EQ( 12, remainder );
    EXPECT_EQ( 0, bitset.count() );
    EXPECT_EQ( results, testSet );

}

// Test resizing the container up and down
TEST(HierarchicalBitset, Resizing)
{
    HierarchicalBitset bitset(20);

    bitset.set(5);

    // Resize 20 -> 30 with fill enabled, creates 10 new bits starting at 20
    bitset.resize(30, true);

    EXPECT_TRUE( bitset.test(5) );
    EXPECT_EQ( 11, bitset.count() );

    // Resize down to 6, this removes the 10 bits
    bitset.resize(6);

    EXPECT_TRUE( bitset.test(5) );
    EXPECT_EQ( 1, bitset.count() );
}

// Test Iterators
TEST(HierarchicalBitset, Iterators)
{
    HierarchicalBitset bitset(128);

    bitset.set(0);
    bitset.set(42);
    bitset.set(127);

    EXPECT_EQ(0, *std::begin(bitset));

    bitset.reset(0);

    auto it = std::begin(bitset);

    EXPECT_EQ(42, *it);

    std::advance(it, 1);

    EXPECT_EQ( 127, *it );

    std::advance(it, 1);

    EXPECT_EQ( std::end(bitset), it );
}

TEST(HierarchicalBitset, EmptyContainer)
{
    HierarchicalBitset bitset;

    EXPECT_EQ(bitset.size(), 0);
    EXPECT_EQ(bitset.data(), nullptr);

    for (std::size_t _ : bitset)
    {
        ASSERT_TRUE(false);
    }
}

TEST(HierarchicalBitset, RangeLoop)
{
    constexpr size_t const sc_max = 13370;
    constexpr size_t const sc_seed = 69;

    std::vector<int> const testSet = random_ascending(sc_seed, sc_max);

    HierarchicalBitset bitset(sc_max);

    for (int i : testSet)
    {
        bitset.set(i);
    }

    int i = 0;
    for (size_t num : bitset)
    {
        EXPECT_EQ(num, testSet[i]);
        i ++;
    }

    EXPECT_EQ(i, testSet.size());
}
