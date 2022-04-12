/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include <iterator>

#include "../utility/bitmath.hpp"

namespace lgrn
{

#if 0

/**
 * @brief TODO
 */
template<typename IT_T>
class BitIterator
{
    using int_t = typename std::iterator_traits<IT_T>::value_type;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = bool;
    using pointer           = void;
    using reference         = void;

    constexpr BitIterator() noexcept = default;
    constexpr BitIterator(IT_T it, int_t bit) noexcept
     : m_it(it)
     , m_posMask(int_t(0x1) << bit)
    { };
    constexpr BitIterator(BitIterator const& copy) noexcept = default;
    constexpr BitIterator(BitIterator&& move) noexcept = default;

    constexpr BitIterator operator=(BitIterator&& move) noexcept = default;

private:
    IT_T m_it;
    int_t m_posMask; // single bit denotes position. ie 00000100 for pos 2
};

#endif

/**
 * @brief Iterate ones bits or zeros bits as a set of integer position
 */
template<typename IT_T, typename ITB_T, bool ONES,
         typename ADVANCE_SKIP_T>
class BitPosIterator : private ADVANCE_SKIP_T // allow EBO
{
    using int_t = typename std::iterator_traits<IT_T>::value_type;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");
public:
    // TODO: consider bi-directional?
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::size_t;
    using pointer           = void;
    using reference         = void;

    constexpr BitPosIterator() noexcept = default;
    constexpr BitPosIterator(ADVANCE_SKIP_T&& skip, IT_T begin, ITB_T end, IT_T it, int_t bit, std::size_t dist) noexcept
     : ADVANCE_SKIP_T   {skip}
     , m_it             {it}
     , m_begin          {begin}
     , m_end            {end}
     , m_distance       {dist}
    {
        if ( (it != end) )
        {
            m_block = (int_t(int_t(int_t(~int_t(0x0)) << bit) & read()));

            // increment to first valid bit
            if (m_block == 0)
            {
                ++(*this);
            }
        }
    };
    constexpr BitPosIterator(BitPosIterator const& copy) noexcept = default;
    constexpr BitPosIterator(BitPosIterator&& move) noexcept = default;

    constexpr BitPosIterator& operator=(BitPosIterator const& copy) noexcept = default;
    constexpr BitPosIterator& operator=(BitPosIterator&& move) noexcept = default;

    constexpr BitPosIterator& operator++() noexcept
    {
        // Remove LSB
        m_block = m_block & (m_block - 1);

        // move to next int if no more bits left
        if (m_block == 0)
        {
            std::size_t const dist = ADVANCE_SKIP_T::operator()(m_begin, m_end, m_it);
            m_distance += dist;
            m_block = (m_it != m_end) ? read() : 0;
        }

        return *this;
    }

    constexpr friend bool operator==(BitPosIterator const& lhs,
                                     BitPosIterator const& rhs) noexcept
    {
        return (lhs.m_it == rhs.m_it) && (lhs.m_block == rhs.m_block);
    };

    constexpr friend bool operator!=(BitPosIterator const& lhs,
                                     BitPosIterator const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    constexpr value_type operator*() const noexcept
    {
        return m_distance + ctz(m_block);
    }

private:

    constexpr int_t read()
    {
        if constexpr (ONES)
        {
            return *m_it;
        }
        else
        {
            // simply read inverted when iterating zeros
            return ~int_t(*m_it);
        }
    }

    IT_T m_begin;
    ITB_T m_end;

    IT_T m_it;
    std::size_t m_distance{0};
    int_t m_block{0};
};


} // namespace lgrn
