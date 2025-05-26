/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "iterator_pair.hpp"            // for IteratorPair
#include "bit_iterator.hpp"
#include "../utility/bitmath.hpp"
#include "../utility/asserts.hpp"       // for LGRN_ASSERTMV

#include <algorithm>
#include <bitset>
#include <type_traits>

namespace lgrn
{

// Inheritance here is mostly used for Empty Base Optimization, not an "is-a" relationship

/**
 * @brief Mixin that adapts a bit interface around an integer range
 */
template <typename RANGE_T>
class BitView : private RANGE_T
{
    using RangeIter_t  = decltype(std::cbegin(std::declval<RANGE_T&>()));
    using RangeSntl_t  = decltype(std::cend  (std::declval<RANGE_T&>()));

public:
    using IntRange_t    = RANGE_T;

    using ZerosIter_t   = BitPosIterator< RangeIter_t, RangeSntl_t, false >;
    using ZerosSntl_t   = typename ZerosIter_t::Sentinel;

    using OnesIter_t    = BitPosIterator< RangeIter_t, RangeSntl_t, true >;
    using OnesSntl_t    = typename OnesIter_t::Sentinel;

    using int_t         = std::remove_cv_t<typename std::iterator_traits<RangeIter_t>::value_type>;

private:
    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");
    static constexpr int smc_bitSize = sizeof(int_t) * 8;

public:

    using OnesRangeView_t  = BitPosRangeView<RangeIter_t, RangeSntl_t, OnesIter_t,  OnesSntl_t,  int_t>;
    using ZerosRangeView_t = BitPosRangeView<RangeIter_t, RangeSntl_t, ZerosIter_t, ZerosSntl_t, int_t>;

    static constexpr std::size_t int_bitsize() noexcept { return smc_bitSize; }

    constexpr BitView()                                     = default;
    constexpr BitView(BitView const& copy)                  = default;
    constexpr BitView(BitView&& move) noexcept              = default;
    constexpr BitView(RANGE_T range) : RANGE_T(range) { }

    constexpr BitView& operator=(BitView const& copy)       = default;
    constexpr BitView& operator=(BitView&& move) noexcept   = default;

    constexpr bool test(std::size_t bit) const noexcept;

    constexpr void set(std::size_t bit) noexcept;
    constexpr void set() noexcept;
    constexpr void reset(std::size_t bit) noexcept;
    constexpr void reset() noexcept;

    constexpr std::size_t size() const noexcept;
    constexpr std::size_t count() const noexcept;

    /**
     * @brief Return a range type (with begin/end functions) used to iterate positions of ones bits
     */
    constexpr OnesRangeView_t ones() const noexcept
    { return { std::cbegin(ints()), std::cend(ints()) }; }

    /**
     * @brief Return a range type (with begin/end functions) used to iterate positions of zeros bits
     */
    constexpr ZerosRangeView_t zeros() const noexcept
    { return { std::cbegin(ints()), std::cend(ints()) }; }

    constexpr IntRange_t&       ints()       noexcept { return static_cast<IntRange_t&>(*this); }
    constexpr IntRange_t const& ints() const noexcept { return static_cast<IntRange_t const&>(*this); }
};

template <typename RANGE_T>
constexpr bool BitView<RANGE_T>::test(std::size_t bit) const noexcept
{
    LGRN_ASSERTMV(bit < size(), "Bit position out of range", bit, size());

    std::size_t const block    = bit / smc_bitSize;
    std::size_t const blockBit = bit % smc_bitSize;

    return bit_test(*std::next(std::begin(ints()), block), blockBit);
}

template <typename RANGE_T>
constexpr void BitView<RANGE_T>::set(std::size_t bit) noexcept
{
    LGRN_ASSERTMV(bit < size(), "Bit position out of range", bit, size());

    std::size_t const block = bit / smc_bitSize;
    std::size_t const blockBit = bit % smc_bitSize;

    *std::next(std::begin(ints()), block) |= int_t(0x1) << blockBit;
}

template <typename RANGE_T>
constexpr void BitView<RANGE_T>::set() noexcept
{
    std::fill(std::begin(ints()), std::end(ints()), ~int_t(0x0));
}

template <typename RANGE_T>
constexpr void BitView<RANGE_T>::reset(std::size_t bit) noexcept
{
    LGRN_ASSERTMV(bit < size(), "Bit position out of range", bit, size());

    std::size_t const block = bit / smc_bitSize;
    std::size_t const blockBit = bit % smc_bitSize;

    *std::next(std::begin(ints()), block) &= ~(int_t(0x1) << blockBit);
}

template <typename RANGE_T>
constexpr void BitView<RANGE_T>::reset() noexcept
{
    std::fill(std::begin(ints()), std::end(ints()), int_t(0x0));
}

template <typename RANGE_T>
constexpr std::size_t BitView<RANGE_T>::size() const noexcept
{
    return std::distance(std::begin(ints()), std::end(ints())) * smc_bitSize;
}

template <typename RANGE_T>
constexpr std::size_t BitView<RANGE_T>::count() const noexcept
{
    std::size_t total = 0;
    auto it = std::begin(ints());
    while (it != std::end(ints()))
    {
        total += std::bitset<smc_bitSize>(*it).count();
        std::advance(it, 1);
    }
    return total;
}

template <typename ITER_T, typename SNTL_T>
constexpr auto bit_view(ITER_T first, SNTL_T last)
{
    return BitView(IteratorPair(first, last));
}

template <typename RANGE_T>
constexpr auto bit_view(RANGE_T& rRange)
{
    return bit_view(std::begin(rRange), std::end(rRange));
}

} // namespace lgrn
