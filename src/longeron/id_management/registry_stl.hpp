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
 * @brief STL-style owning IdRegistry with underlying std::vector
 */
template<typename ID_T, bool NO_AUTO_RESIZE = false,
         typename BITVIEW_T = BitView< std::vector<uint64_t> > >
class IdRegistryStl : private BitViewIdRegistry<BITVIEW_T, ID_T>
{
    using underlying_t = BitViewIdRegistry<BITVIEW_T, ID_T>;

    friend underlying_t;

public:

    IdRegistryStl() = default;
    IdRegistryStl(underlying_t reg) : underlying_t(reg) { }

    using underlying_t::bitview;
    using underlying_t::capacity;
    using underlying_t::size;
    using underlying_t::remove;
    using underlying_t::exists;

    /**
     * @brief Create a single ID
     *
     * @return A newly created ID
     */
    [[nodiscard]] ID_T create()
    {
        ID_T output{ id_null<ID_T>() };
        create(&output, &output + 1);
        return output;
    }

    /**
     * @brief Create multiple IDs
     *
     * @param first [out]
     * @param last  [in]
     */
    template<typename IT_T, typename ITB_T>
    IT_T create(IT_T first, ITB_T last);

    void reserve(std::size_t n)
    {
        // may need some considerations when hierarchical bitsets are re-added

        // Resize with all new bits set, as 1 is for free Id
        vec().resize(lgrn::div_ceil(n, underlying_t::bitview().int_bitsize()),
                     ~uint64_t(0));
    }

    auto& vec() noexcept { return underlying_t::bitview().ints(); }
    auto const& vec() const noexcept { return underlying_t::bitview().ints(); }
};


template<typename ID_T, bool NO_AUTO_RESIZE, typename BITVIEW_T>
template<typename IT_T, typename ITB_T>
IT_T IdRegistryStl<ID_T, NO_AUTO_RESIZE, BITVIEW_T>::create(IT_T first, ITB_T last)
{
    if constexpr (NO_AUTO_RESIZE)
    {
        return underlying_t::create(first, last);
    }
    else
    {
        while (true)
        {
            first = underlying_t::create(first, last);

            if (first != last) [[unlikely]]
            {
                // out of space, not all IDs were created

                // initialize to all bits set, as ones are for free IDs
                // semi-jank way to use vector's automatic reallocation logic
                vec().push_back(~uint64_t(0));
                vec().resize(vec().capacity(), ~uint64_t(0));

            }
            else [[likely]]
            {
                break;
            }
        }
        return first;
    }
}


} // namespace lgrn
