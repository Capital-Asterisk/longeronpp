/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#include <longeron/containers/intarray_multimap.hpp>
#include <longeron/id_management/registry_stl.hpp>

#include <gtest/gtest.h>

#include <array>
#include <random>
#include <unordered_map>
#include <vector>

using lgrn::IdRegistryStl;
using lgrn::IntArrayMultiMap;

using id_t = unsigned int;

// Basic usage
TEST(IntArrayMultiMap, Basic)
{
    // allocate with 16 max floats, and 4 IDs from 0 to 3
    IntArrayMultiMap<id_t, float> multimap(16, 4);

    // Emplace 3 2-element partitions
    multimap.emplace(0, {1.0f, 2.0f});
    multimap.emplace(1, {3.0f, 4.0f});
    multimap.emplace(2, {5.0f, 6.0f});

    // IDs 0 to 2 exists, 3 doesn't exist
    EXPECT_TRUE(multimap.contains(0));
    EXPECT_TRUE(multimap.contains(2));
    EXPECT_FALSE(multimap.contains(3));

    // Verify stored values
    EXPECT_EQ(multimap[0][0], 1.0f);
    EXPECT_EQ(multimap[2][1], 6.0f);

    multimap.erase(1);

    // Partition at id 1 is erased
    EXPECT_FALSE(multimap.contains(1));

    multimap.pack();

    // Packing should affect stored values
    EXPECT_EQ(multimap[0][0], 1.0f);
    EXPECT_EQ(multimap[2][1], 6.0f);

    // const access
    auto const& multimapConst = multimap;

    EXPECT_EQ(multimapConst[0][0], 1.0f);
    EXPECT_EQ(multimapConst[2][1], 6.0f);
}

using Shared_t = std::shared_ptr<float>;

Shared_t shared(float f) { return std::make_shared<float>(f); }

// Test ownership using shared pointers.
TEST(IntArrayMultiMap, Ownership)
{
    // Note that this is not the intended use for this container. Using shared
    // pointers ruin the contiguousness of the stored data; here they are only
    // used to verify the container is properly managing data ownership

    Shared_t const dataA{shared(1.0f)};

    std::array<Shared_t, 4> const dataB{
        shared(2.0f), shared(3.0f), shared(4.0f), shared(5.0f)
    };

    std::array<Shared_t, 5> const dataC{
        shared(6.0f), shared(7.0f), shared(8.0f), shared(9.0f), shared(10.0f)
    };

    // just making sure
    EXPECT_EQ(dataA.use_count(), 1);
    EXPECT_EQ(dataB[0].use_count(), 1);

    {
        IntArrayMultiMap<id_t, Shared_t> multimap(32, 8);

        // Insert multiple copies
        multimap.emplace(0, {dataA});
        multimap.emplace(1, dataB.begin(), dataB.end());
        multimap.emplace(2, dataB.begin(), dataB.end());
        multimap.emplace(3, dataB.begin(), dataB.end());
        multimap.emplace(4, dataC.begin(), dataC.end());
        multimap.emplace(5, dataC.begin(), dataC.end());

        // Shared pointers should now have more users
        // Testing only the first and last elements should be sufficient
        EXPECT_EQ(dataA.use_count(), 2);
        EXPECT_EQ(dataB[0].use_count(), 4);
        EXPECT_EQ(dataB[3].use_count(), 4);
        EXPECT_EQ(dataC[0].use_count(), 3);
        EXPECT_EQ(dataC[4].use_count(), 3);

        multimap.erase(2);

        // 2 containing dataB deleted, users reduced
        EXPECT_EQ(dataB[0].use_count(), 3);
        EXPECT_EQ(dataB[3].use_count(), 3);

        multimap.pack();

        // pack shouldn't change anything
        EXPECT_EQ(dataA.use_count(), 2);
        EXPECT_EQ(dataB[0].use_count(), 3);
        EXPECT_EQ(dataB[3].use_count(), 3);
        EXPECT_EQ(dataC[0].use_count(), 3);
        EXPECT_EQ(dataC[4].use_count(), 3);

        multimap.data_reserve(48);

        // neither should resize
        EXPECT_EQ(dataA.use_count(), 2);
        EXPECT_EQ(dataB[0].use_count(), 3);
        EXPECT_EQ(dataB[3].use_count(), 3);
        EXPECT_EQ(dataC[0].use_count(), 3);
        EXPECT_EQ(dataC[4].use_count(), 3);

    } // destruct multimap

    EXPECT_EQ(dataA.use_count(), 1);
    EXPECT_EQ(dataB[0].use_count(), 1);
    EXPECT_EQ(dataB[3].use_count(), 1);
    EXPECT_EQ(dataC[0].use_count(), 1);
    EXPECT_EQ(dataC[4].use_count(), 1);
}

using Unique_t = std::unique_ptr<float>;

TEST(IntArrayMultiMap, UniqueOwnership)
{
    IntArrayMultiMap<id_t, Unique_t> multimap(4, 2);

    // default construct by passing in size, then move in

    *multimap.emplace(0, 1) = std::make_unique<float>(96.0f);

    Unique_t data = std::make_unique<float>(69.0f);
    Unique_t &rPrtn = *multimap.emplace(1, 1);
    rPrtn = std::move(data);

    // expect that the data has been moved away
    EXPECT_FALSE(bool(data));

    // erasing 0 and packing moves id 1 internally
    multimap.erase(0);
    multimap.pack();

    EXPECT_EQ(*multimap[1][0], 69.0f);
}

// Repetitively delete and create random-sized partitions
// Compare against an std::unordered_map< ..., std::vector<...> >
TEST(IntArrayMultiMap, RandomCreationAndDeletion)
{
    constexpr int const sc_seed         = 69;
    constexpr int const sc_repetitions  = 32;

    constexpr int const sc_idMax        = 256;

    // Partitions to create per iteration
    constexpr int const sc_createMin    = 10;
    constexpr int const sc_createMax    = 70;

    // Sizes of partitions to create
    constexpr int const sc_prtnMin      = 1;
    constexpr int const sc_prtnMax      = 10;

    constexpr int const sc_valueMin     = -99999;
    constexpr int const sc_valueMax     = 99999;

    std::mt19937 gen(sc_seed);
    std::uniform_int_distribution<int> distCreate(sc_createMin, sc_createMax);
    std::uniform_int_distribution<int> distPrtnSize(sc_prtnMin, sc_prtnMax);
    std::uniform_int_distribution<int> distValue(sc_valueMin, sc_valueMax);
    std::uniform_int_distribution<int> distFlip(0, 1);

    std::unordered_map< id_t, std::vector<int> > control;
    IntArrayMultiMap<id_t, int> multimap(sc_idMax * sc_createMax, sc_idMax);
    IdRegistryStl<id_t, true> ids;
    ids.reserve(sc_idMax);

    for (int i = 0; i < sc_repetitions; i ++)
    {
        // Create a random number of partitions

        int const toCreate = distCreate(gen);

        for (int j = 0; j < toCreate; j ++)
        {
            int const prtnSize = distPrtnSize(gen);
            id_t const id = ids.create();

            std::vector<int> values(prtnSize);
            std::generate(values.begin(), values.end(),
                          [&distValue, &gen] { return distValue(gen); });

            multimap.emplace(id, values.cbegin(), values.cend());
            control.emplace(id, std::move(values));
        }

        // Remove about half of them

        std::vector<id_t> toErase;

        // No iterators for IntArrayMultiMap or IdRegistry yet,
        // Use control to get existing IDs
        for (auto const & [id, _] : control)
        {
            if (distFlip(gen) == 1)
            {
                toErase.push_back(id);
            }
        }

        for (id_t id : toErase)
        {
            multimap.erase(id);
            control.erase(id);
            ids.remove(id);
        }

        multimap.pack();

        // Check if all values still match

        for (auto const & [id, controlValues] : control)
        {
            for (int j = 0; j < controlValues.size(); j ++)
            {
                EXPECT_EQ(controlValues[j], multimap[id][j]);
            }
        }
    }
}


