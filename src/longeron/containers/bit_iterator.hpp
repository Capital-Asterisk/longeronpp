/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include <iterator>

#include "../utility/bitmath.hpp"

namespace lgrn
{

/**
 * @brief
 */
template<typename INT_IT_T>
class BitIterator
{
    using int_t = typename std::iterator_traits<INT_IT_T>::value_type;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = bool;
    using pointer           = void;
    using reference         = void;

    constexpr BitIterator() noexcept = default;
    constexpr BitIterator(INT_IT_T it, int_t bit) noexcept
     : m_it(it)
     , m_posMask(int_t(0x1) << bit)
    { };
    constexpr BitIterator(BitIterator const& copy) noexcept = default;
    constexpr BitIterator(BitIterator&& move) noexcept = default;

    constexpr BitIterator operator=(BitIterator&& move) noexcept = default;

private:
    INT_IT_T m_it;
    int_t m_posMask; // single bit denotes position. ie 00000100 for pos 2
};

/**
 * @brief Iterate one or zero bits, outputting their positions
 */
template<typename IT_T, typename ITB_T, bool ONES,
         std::size_t(*ADVANCE_SKIP)(IT_T, ITB_T, IT_T&)>
class BitValueIterator
{
    using int_t = typename std::iterator_traits<IT_T>::value_type;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::size_t;
    using pointer           = void;
    using reference         = void;

    constexpr BitValueIterator() noexcept = default;
    constexpr BitValueIterator(IT_T begin, ITB_T end, IT_T it, int_t bit, std::size_t dist) noexcept
     : m_it         {it}
     , m_begin      {begin}
     , m_end        {end}
     , m_distance   {dist}
     , m_bitLSB     {int_t(0x1) << bit}
    { };
    constexpr BitValueIterator(BitValueIterator const& copy) noexcept = default;
    constexpr BitValueIterator(BitValueIterator&& move) noexcept = default;

    constexpr BitValueIterator& operator=(BitValueIterator&& move) noexcept = default;

    constexpr BitValueIterator& operator++() noexcept
    {
        // this fills all upper bits, current LSB bit will be zero
        // ie: 00010000 -> 11100000
        int_t const mask = -(m_bitLSB + m_bitLSB);

        int_t blkMasked = value() & mask;

        // move to next int if no more bits left
        if (blkMasked == 0)
        {
            std::size_t const dist = ADVANCE_SKIP(m_begin, m_end, m_it);
            m_distance += dist;
            blkMasked = (m_it != m_end) ? value() : 1;
        }

        // get LSB only
        m_bitLSB = blkMasked & (-blkMasked);

        return *this;
    }

    constexpr friend bool operator==(BitValueIterator const& lhs,
                                     BitValueIterator const& rhs) noexcept
    {
        return (lhs.m_it == rhs.m_it) && (lhs.m_bitLSB == rhs.m_bitLSB);
    };

    constexpr friend bool operator!=(BitValueIterator const& lhs,
                                     BitValueIterator const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    constexpr value_type operator*() const noexcept
    {
        return m_distance + ctz(m_bitLSB);
    }

private:

    constexpr int_t value()
    {
        if constexpr (ONES)
        {
            return *m_it;
        }
        else
        {
            return ~int_t(*m_it);
        }
    }

    IT_T m_begin;
    ITB_T m_end;

    IT_T m_it;
    std::size_t m_distance;
    int_t m_bitLSB{0}; // single bit denotes position. ie 00000100 for pos 2
};


} // namespace lgrn
