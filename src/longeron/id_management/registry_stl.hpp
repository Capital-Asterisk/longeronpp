/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "bitview_registry.hpp"
#include "../containers/bit_view.hpp"
#include "../utility/bitmath.hpp"

#include <vector>

namespace lgrn
{

/**
 * @brief Manages sequential integer IDs usable as array indices with automatic reallocation. Uses
 *        std::vector<std::uint64_t> internally.
 */
template<typename ID_T, bool NO_AUTO_RESIZE = false,
         typename BITVIEW_T = BitView< std::vector<std::uint64_t> > >
class IdRegistryStl : private BitViewIdRegistry<BITVIEW_T, ID_T>
{
public:
    using Base_t    = BitViewIdRegistry<BITVIEW_T, ID_T>;
    using BitView_t = typename Base_t::Base_t;

    class Generator
    {
        using OnesIter_t = typename BitView_t::OnesIter_t;
    public:
        Generator(IdRegistryStl &rRegistry)
         : iter{rRegistry.bitview().ones().begin()}
         , rRegistry{rRegistry}
        { }

        [[nodiscard]] ID_T create();

        ID_T operator()() { return create(); }

    private:
        OnesIter_t      iter;
        IdRegistryStl   &rRegistry;
    };

    friend Generator;

    IdRegistryStl() noexcept = default;
    IdRegistryStl(Base_t reg) : Base_t{ std::move(reg) } { }

    using Base_t::Base_t;
    using Base_t::begin;
    using Base_t::bitview;
    using Base_t::capacity;
    using Base_t::end;
    using Base_t::exists;
    using Base_t::remove;
    using Base_t::size;

    /**
     * @brief Create a single ID
     *
     * This will automatically reallocate to fit more IDs unless NO_AUTO_RESIZE is enabled.
     *
     * @return New ID created or potentially null only if NO_AUTO_RESIZE is enabled
     */
    [[nodiscard]] ID_T create()
    {
        ID_T output{ id_null<ID_T>() };
        create(&output, &output + 1);
        return output;
    }

    /**
     * @brief Create multiple IDs, store new IDs in a specified range
     *
     * The number of IDs created will be limited by the size of the range.
     *
     * This will automatically reallocate to fit more IDs unless NO_AUTO_RESIZE is enabled.
     *
     * @return Iterator that is one past the last value written to
     */
    template<typename ITER_T, typename SNTL_T>
    ITER_T create(ITER_T first, SNTL_T last);

    /**
     * @brief Create a type used to efficiently create multiple IDs with individual function calls
     */
    Generator generator()
    {
        return Generator{*this};
    }

    void reserve(std::size_t n)
    {
        // Resize with all new bits set, as 1 is for free Id
        vec().resize(lgrn::div_ceil(n, Base_t::bitview().int_bitsize()), ~uint64_t(0));
    }

    [[nodiscard]] constexpr auto&       vec()       noexcept { return Base_t::bitview().ints(); }
    [[nodiscard]] constexpr auto const& vec() const noexcept { return Base_t::bitview().ints(); }

private:

    /**
     * @brief Expand capacity using std::vectors automatic reallocation logic in push_back
     */
    void reserve_auto()
    {
        // initialize to all bits set, as ones are for free IDs
        // semi-jank way to use vector's automatic reallocation logic
        vec().push_back(~uint64_t(0));
        vec().resize(vec().capacity(), ~uint64_t(0));
    }
};


template<typename ID_T, bool NO_AUTO_RESIZE, typename RANGE_T>
template<typename ITER_T, typename SNTL_T>
ITER_T IdRegistryStl<ID_T, NO_AUTO_RESIZE, RANGE_T>::create(ITER_T first, SNTL_T last)
{
    if constexpr (NO_AUTO_RESIZE)
    {
        return Base_t::create(first, last);
    }
    else
    {
        while (true)
        {
            first = Base_t::create(first, last);

            if (first == last)
            {
                break;
            }

            // underlying_t::create didn't could fill the entire range. Out of space!
            // Reallocate then retry.
            reserve_auto();
        }
        return first;
    }
}

template<typename ID_T, bool NO_AUTO_RESIZE, typename RANGE_T>
ID_T IdRegistryStl<ID_T, NO_AUTO_RESIZE, RANGE_T>::Generator::create()
{
    using Sentinel_t = typename Generator::OnesIter_t::Sentinel;

    if (iter == Sentinel_t{}) // If out of space
    {
        if constexpr (NO_AUTO_RESIZE)
        {
            return id_null<ID_T>();
        }
        else
        {
            auto const prevCapacity = rRegistry.capacity();
            rRegistry.reserve_auto();
            iter = rRegistry.bitview().ones().begin_at(prevCapacity);
            LGRN_ASSERT(iter != Sentinel_t{});
        }
    }

    std::size_t const outInt = *iter;
    ++iter;

    rRegistry.bitview().reset(outInt);

    return ID_T{outInt};
}

} // namespace lgrn
