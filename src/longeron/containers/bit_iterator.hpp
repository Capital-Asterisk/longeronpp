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
 * @brief Iterate positions of ones bits or zeros bits within an integer range
 *
 * This works by storing an internal iterator to the specified int range, and saving the current
 * value. Per iteration (operator++), this iterator will scan through the bits of the current value
 * from LSB to MSB. If there's no bits left, then the internal iterator will be repeatedly
 * incremented to search for the next int containing ones or zeros bits.
 *
 * @warning Do not modify the integer range while this iterator is alive.
 */
template<typename ITER_T, typename SNTL_T, bool ONES>
class BitPosIterator
{
    using int_t = typename std::iterator_traits<ITER_T>::value_type;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");

    static constexpr int_t smc_emptyBlock = ONES ? int_t(0) : ~int_t(0);
public:

    // TODO: consider bi-directional?
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::size_t;
    using pointer           = void;
    using reference         = void;

    struct Sentinel { };

    constexpr BitPosIterator() noexcept = default;
    constexpr BitPosIterator(ITER_T it, SNTL_T end, std::size_t dist, int bit) noexcept
     : m_end            {end}
     , m_it             {it}
     , m_distance       {dist}
    {
        if ( (it != end) )
        {
            m_block = (int_t(int_t(int_t(~int_t(0x0)) << bit) & int_iter_value()));

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

    ~BitPosIterator() = default;

    constexpr BitPosIterator& operator++() noexcept
    {
        // Remove LSB
        m_block = m_block & (m_block - 1);

        // move to next int if no more bits left
        if (m_block == 0)
        {
            // Skip empty blocks (no ones or no zero bits)
            do
            {
                ++m_it;
                m_distance += sizeof(int_t) * 8;
            }
            while (m_it != m_end && *m_it == smc_emptyBlock);

            m_block = (m_it != m_end) ? int_iter_value() : 0;
        }

        return *this;
    }

    constexpr value_type operator*() const noexcept
    {
        std::size_t const pos = m_distance + ctz(m_block);
        return pos;
    }

private:

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

    constexpr friend bool operator==(BitPosIterator const& lhs,
                                     Sentinel       const&) noexcept
    {
        return lhs.m_it == lhs.m_end;
    }

    constexpr friend bool operator!=(BitPosIterator const& lhs,
                                     Sentinel       const&) noexcept
    {
        return lhs.m_it != lhs.m_end;
    }

    constexpr int_t int_iter_value()
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

    // End needs to be stored becuase of the "skip forward until we find a bit" behaviour
    SNTL_T          m_end;
    ITER_T          m_it;
    std::size_t     m_distance{~std::size_t(0)};
    int_t           m_block{0};
};


template <typename RANGE_ITER_T, typename RANGE_SNTL_T,
          typename POS_ITER_T,   typename POS_SNTL_T, typename INT_T>
class BitPosRangeView
{
    static constexpr int smc_bitSize = sizeof(INT_T) * 8;
public:

    constexpr BitPosRangeView(RANGE_ITER_T first, RANGE_SNTL_T last)
     : first{ std::move(first) }
     , last { std::move(last) }
    { }

    constexpr POS_ITER_T begin() const noexcept { return POS_ITER_T(first, last, 0, 0); }

    constexpr POS_ITER_T begin_at(std::size_t const bitPos) const noexcept
    {
        auto const intPos    = bitPos / smc_bitSize;
        auto const intBitPos = bitPos % smc_bitSize;

        return POS_ITER_T(std::next(first, intPos), last, smc_bitSize*intPos, intBitPos);
    }

    constexpr POS_SNTL_T end() const noexcept { return POS_SNTL_T{}; }
private:
    RANGE_ITER_T first;
    RANGE_SNTL_T last;
};

} // namespace lgrn
