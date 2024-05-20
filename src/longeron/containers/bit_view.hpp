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

/**
 * @brief Mixin that adapts a bit interface around an integer range
 */
template <typename RANGE_T>
class BitView : private RANGE_T
{
    using iter_t = decltype(std::cbegin(std::declval<RANGE_T&>()));
    using sntl_t = decltype(std::cend  (std::declval<RANGE_T&>()));

    using int_t = std::remove_cv_t<typename std::iterator_traits<iter_t>::value_type>;

    static constexpr int smc_bitSize = sizeof(int_t) * 8;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");

    template <bool ONES>
    class BitViewValues
    {
        using ValueIt_t = BitPosIterator< iter_t, sntl_t, ONES >;
    public:

        constexpr BitViewValues(BitView const& view)
         : first{ std::cbegin(view.ints()) }
         , last { std::cend  (view.ints()) }
        { }

        constexpr ValueIt_t begin() const noexcept
        {
            return ValueIt_t(last, first, 0, 0);
        }

        constexpr BitPosSentinel end() const noexcept
        {
            return BitPosSentinel{};
        }
    private:
        iter_t first;
        sntl_t last;
    };

public:

    static constexpr std::size_t int_bitsize() noexcept { return smc_bitSize; }

    constexpr BitView() = default;
    constexpr BitView(RANGE_T range) : RANGE_T(range) { }

    constexpr bool test(std::size_t bit) const noexcept;

    constexpr void set(std::size_t bit) noexcept;
    constexpr void set() noexcept;
    constexpr void reset(std::size_t bit) noexcept;
    constexpr void reset() noexcept;

    constexpr std::size_t size() const noexcept;
    constexpr std::size_t count() const noexcept;

    constexpr BitViewValues<true> ones() const noexcept { return {*this}; }
    constexpr BitViewValues<false> zeros() const noexcept { return {*this}; }

    constexpr RANGE_T& ints() noexcept { return static_cast<RANGE_T&>(*this); }
    constexpr RANGE_T const& ints() const noexcept { return static_cast<RANGE_T const&>(*this); }
};

template <typename RANGE_T>
constexpr bool BitView<RANGE_T>::test(std::size_t bit) const noexcept
{
    LGRN_ASSERTMV(bit < size(), "Bit position out of range", bit, size());

    std::size_t const block = bit / smc_bitSize;
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
    iter_t it = std::begin(ints());
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
