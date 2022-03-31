/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "bit_iterator.hpp"

#include <type_traits>

namespace lgrn
{


/**
 * @brief Adapts a bit interface around an integer span-like type
 *
 * Not intended for storage purposes.
 */
template <typename IT_T, typename ITB_T>
class BitView
{
    using int_t = typename std::iterator_traits<IT_T>::value_type;

    static constexpr int smc_bitSize = sizeof(int_t) * 8;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");

    template<int_t VALUE>
    static bool stupid_test(IT_T const& it)
    {
        return (*it == VALUE);
    }

    template<int_t VALUE>
    static std::size_t advance_skip([[maybe_unused]] IT_T begin, IT_T end, IT_T& it)
    {
        std::size_t dist = 0;
        do
        {
            ++it;
            dist += smc_bitSize;
        }
        while (it != end && *it == VALUE);
        return dist;
    }

    template <bool ONES>
    class BitViewValues
    {
        using ValueIt_t = std::conditional_t<ONES,
                BitValueIterator< IT_T, ITB_T, ONES, advance_skip<0x0> >,
                BitValueIterator< IT_T, ITB_T, ONES, advance_skip<~int_t(0x0)> > >;

    public:

        constexpr BitViewValues(BitView const *pView)
         : m_pView{pView}
        { }

        constexpr ValueIt_t begin() const noexcept
        {
            return {m_pView->m_begin, m_pView->m_end, m_pView->m_begin, 0, 0};
        }
        constexpr ValueIt_t end() const noexcept
        {
            std::size_t const dist = std::distance(m_pView->m_begin, m_pView->m_end) * smc_bitSize;
            return {m_pView->m_begin, m_pView->m_end, m_pView->m_end, 0, dist};
        }
    private:
        BitView const *m_pView;
    };

public:

    constexpr BitView(IT_T first, ITB_T last)
     : m_begin{first}
     , m_end{last}
    { }

    constexpr void set(std::size_t bit) noexcept;
    constexpr void set() noexcept;
    constexpr void reset(std::size_t bit) noexcept;
    constexpr void reset() noexcept;
    constexpr bool test(std::size_t bit) noexcept;

    constexpr std::size_t size() const noexcept;
    constexpr BitViewValues<true> const ones() const noexcept { return this; }
    constexpr BitViewValues<false> const zeros() const noexcept { return this; }

private:
    IT_T m_begin;
    ITB_T m_end;
};


template <typename RANGE_T>
constexpr auto bit_range(RANGE_T& rRange)
{
    return BitView(std::begin(rRange), std::end(rRange));
}

} // namespace lgrn
