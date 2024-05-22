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

    ASSERT_EQ(std::size_t(idA), 0);
    ASSERT_EQ(std::size_t(idB), 1);
    ASSERT_EQ(std::size_t(idC), 2);
    ASSERT_TRUE( registry.exists(idA) );
    ASSERT_TRUE( registry.exists(idB) );
    ASSERT_TRUE( registry.exists(idC) );
    ASSERT_EQ( registry.size(), 3 );

    registry.remove( idB );

    ASSERT_TRUE(  registry.exists(idA) );
    ASSERT_FALSE( registry.exists(idB) );
    ASSERT_TRUE(  registry.exists(idC) );
    ASSERT_EQ( registry.size(), 2 );

    idB = registry.create();

    ASSERT_EQ(std::size_t(idB), 1);
    ASSERT_TRUE( registry.exists(idA) );
    ASSERT_TRUE( registry.exists(idB) );
    ASSERT_TRUE( registry.exists(idC) );
    ASSERT_EQ( registry.size(), 3 );

    std::array<Id, 128> idsArray{};
    registry.create(std::begin(idsArray), std::end(idsArray));

    for (Id id : idsArray)
    {
        EXPECT_TRUE( registry.exists(id) );
    }

    ASSERT_TRUE(std::equal(idsArray.begin(), idsArray.end(), std::next(registry.begin(), 3)));

    ASSERT_EQ( registry.size(), 3+128 );
}

// IdRegistryStl generator
TEST(IdRegistry, Generator)
{
    lgrn::IdRegistryStl<Id> registry;

    auto generator = registry.generator();

    for (std::size_t expectedId = 0; expectedId < 10000; ++expectedId)
    {
        Id const id = generator.create();

        ASSERT_EQ(std::size_t(id), expectedId);
        ASSERT_TRUE( registry.exists(id) );
    }

    ASSERT_EQ( registry.size(), 10000 );
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
