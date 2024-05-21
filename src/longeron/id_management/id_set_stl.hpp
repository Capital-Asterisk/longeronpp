/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "bitview_id_set.hpp"

#include <vector>

namespace lgrn
{

/**
 * @brief An STL-style container to store a set of ID types. Uses std::vector<std::uint64_t>
 *        internally.
 *
 * No automatic reallocations. Use \c reserve();
 */
template<typename ID_T>
class IdSetStl : public BitViewIdSet<std::vector<uint64_t>, ID_T>
{
public:
    using base_t    = BitViewIdSet<std::vector<uint64_t>, ID_T>;
    using bitview_t = typename base_t::base_t;

    [[nodiscard]] constexpr auto&       vec()       noexcept { return base_t::bitview().ints(); }
    [[nodiscard]] constexpr auto const& vec() const noexcept { return base_t::bitview().ints(); }

    void resize(std::size_t n)
    {
        vec().resize(lgrn::div_ceil(n, base_t::bitview().int_bitsize()), 0);
    }
};


} // namespace lgrn
