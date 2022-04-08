/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "bit_iterator.hpp"
#include "../utility/bitmath.hpp"

#include <type_traits>

namespace lgrn
{

/**
 * @brief Adapts a bit interface around an integer range
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
    struct advance_skip
    {
        std::size_t operator()([[maybe_unused]] IT_T begin, ITB_T end, IT_T& it) const noexcept
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
    };

    template <bool ONES>
    class BitViewValues
    {
        using ValueIt_t = std::conditional_t<ONES,
                BitPosIterator< IT_T, ITB_T, ONES, advance_skip<int_t(0)> >,
                BitPosIterator< IT_T, ITB_T, ONES, advance_skip<int_t(~int_t(0))> > >;

    public:

        constexpr BitViewValues(BitView const* pView)
         : m_pView{pView}
        { }

        constexpr ValueIt_t begin() const noexcept
        {
            return {{}, m_pView->m_begin, m_pView->m_end, m_pView->m_begin, 0, 0};
        }
        constexpr ValueIt_t end() const noexcept
        {
            std::size_t const dist = std::distance(m_pView->m_begin, m_pView->m_end) * smc_bitSize;
            return {{}, m_pView->m_begin, m_pView->m_end, m_pView->m_end, 0, dist};
        }
    private:
        BitView const *m_pView;
    };

public:

    constexpr BitView(IT_T first, ITB_T last)
     : m_begin{first}
     , m_end{last}
    { }

    constexpr bool test(std::size_t bit) const noexcept;

    constexpr void set(std::size_t bit) noexcept;
    constexpr void set() noexcept;
    constexpr void reset(std::size_t bit) noexcept;
    constexpr void reset() noexcept;

    constexpr std::size_t size() const noexcept;
    constexpr BitViewValues<true> const ones() const noexcept { return this; }
    constexpr BitViewValues<false> const zeros() const noexcept { return this; }

private:
    IT_T m_begin;
    ITB_T m_end;
};

template <typename IT_T, typename ITB_T>
constexpr bool BitView<IT_T, ITB_T>::test(std::size_t bit) const noexcept
{
    std::size_t const block = bit / smc_bitSize;
    std::size_t const blockBit = bit % smc_bitSize;

    return bit_test(std::next(m_begin, block), blockBit);
}

template <typename IT_T, typename ITB_T>
constexpr void BitView<IT_T, ITB_T>::set(std::size_t bit) noexcept
{
    std::size_t const block = bit / smc_bitSize;
    std::size_t const blockBit = bit % smc_bitSize;

    *std::next(m_begin, block) |= int_t(0x1) << blockBit;
}

template <typename IT_T, typename ITB_T>
constexpr void BitView<IT_T, ITB_T>::set() noexcept
{
    std::fill(m_begin, m_end, ~int_t(0x0));
}

template <typename IT_T, typename ITB_T>
constexpr void BitView<IT_T, ITB_T>::reset(std::size_t bit) noexcept
{
    std::size_t const block = bit / smc_bitSize;
    std::size_t const blockBit = bit % smc_bitSize;

    *std::next(m_begin, block) &= ~(int_t(0x1) << blockBit);
}

template <typename IT_T, typename ITB_T>
constexpr void BitView<IT_T, ITB_T>::reset() noexcept
{
    std::fill(m_begin, m_end, int_t(0x0));
}


template <typename RANGE_T>
constexpr auto bit_view(RANGE_T& rRange)
{
    return BitView(std::begin(rRange), std::end(rRange));
}

} // namespace lgrn
