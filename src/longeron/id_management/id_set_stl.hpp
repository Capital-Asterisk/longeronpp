/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "bitview_id_set.hpp"

#include "../containers/bit_view.hpp"
#include "../utility/bitmath.hpp"

#include <cstdint>

#include <vector>

namespace lgrn
{

/**
 * @brief An STL-style container to store a set of ID types. Uses std::vector<std::uint64_t>
 *        internally.
 *
 * No automatic reallocations. Use \c reserve();
 */
template<typename ID_T, typename BITVIEW_T = lgrn::BitView< std::vector<std::uint64_t> > >
class IdSetStl : public BitViewIdSet<BITVIEW_T, ID_T>
{
public:
    using Base_t    = BitViewIdSet<BITVIEW_T, ID_T>;
    using bitview_t = typename Base_t::Base_t;

    [[nodiscard]] constexpr auto&       vec()       noexcept { return Base_t::bitview().ints(); }
    [[nodiscard]] constexpr auto const& vec() const noexcept { return Base_t::bitview().ints(); }

    void resize(std::size_t n)
    {
        vec().resize(lgrn::div_ceil(n, Base_t::bitview().int_bitsize()), 0);
    }
};


} // namespace lgrn
