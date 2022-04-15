/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#include <longeron/id_management/registry_stl.hpp>

#include <gtest/gtest.h>

#include <array>
#include <random>

enum class Id : uint64_t { };

// Test lgrn::underlying_int_type
static_assert(std::is_same_v<lgrn::underlying_int_type_t<int>, int>);
static_assert(std::is_same_v<lgrn::underlying_int_type_t<Id>, uint64_t>);

// Basic intended use test of managing IDs
TEST(IdRegistry, ManageIds)
{
    lgrn::IdRegistryStl<Id> registry;

    Id idA = registry.create();
    Id idB = registry.create();
    Id idC = registry.create();

    EXPECT_TRUE( registry.exists(idA) );
    EXPECT_TRUE( registry.exists(idB) );
    EXPECT_TRUE( registry.exists(idC) );

    EXPECT_EQ( registry.size(), 3 );

    ASSERT_NE( idA, idB );
    ASSERT_NE( idB, idC );
    ASSERT_NE( idC, idA );

    registry.remove( idB );

    EXPECT_TRUE( registry.exists(idA) );
    EXPECT_FALSE( registry.exists(idB) );
    EXPECT_TRUE( registry.exists(idC) );

    EXPECT_EQ( registry.size(), 2 );

    std::array<Id, 32> ids;
    registry.create(std::begin(ids), std::end(ids));

    for (Id id : ids)
    {
        EXPECT_TRUE( registry.exists(id) );
    }

    EXPECT_EQ( registry.size(), 34 );
}

// A more chaotic test of repeated adding a random amount of new IDs then
// deleting half of them randomly
TEST(IdRegistry, RandomCreationAndDeletion)
{
    constexpr size_t const sc_seed = 69;
    constexpr size_t const sc_createMax = 100;
    constexpr size_t const sc_createMin = 60;
    constexpr size_t const sc_repetitions = 32;

    lgrn::IdRegistryStl<Id> registry;

    std::set<Id> idSet;

    std::mt19937 gen(sc_seed);
    std::uniform_int_distribution<int> distCreate(sc_createMin, sc_createMax);
    std::uniform_int_distribution<int> distFlip(0, 1);


    for (unsigned int i = 0; i < sc_repetitions; i ++)
    {
        // Create a bunch of new IDs

        size_t const toCreate = distCreate(gen);
        std::array<Id, sc_createMax> newIds;

        registry.create(std::begin(newIds), std::begin(newIds) + toCreate);
        idSet.insert(std::begin(newIds), std::begin(newIds) + toCreate);

        // Remove about half of the IDs

        std::vector<Id> toErase;

        for (Id id : idSet)
        {
            if (distFlip(gen) == 1)
            {
                registry.remove(id);
                toErase.push_back(id);

                EXPECT_FALSE( registry.exists(id) );
            }
        }

        for (Id id : toErase)
        {
            idSet.erase(id);
        }

        // Check if all IDs are still valid

        for (Id id : idSet)
        {
            EXPECT_TRUE( registry.exists(id) );
        }

        EXPECT_EQ( idSet.size(), registry.size() );
    }
}
