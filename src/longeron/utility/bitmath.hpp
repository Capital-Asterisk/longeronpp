/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include <cstdint>
#include <type_traits>


#if defined(_MSC_VER)
    #include <intrin.h>
#endif

namespace lgrn
{

#ifdef __GNUC__

    inline int ctz(uint64_t a) noexcept { return __builtin_ctzll(a); }

#elif defined(_MSC_VER)

    inline int ctz(uint64_t a) noexcept
    {
        unsigned long int b = 0;
        _BitScanForward64(&b, a);
        return b;
    }

#elif

    static_assert(false, "Missing ctz 'count trailing zeros' implementation for this compiler");

#endif

/**
 * @brief Divide two integers and round up
 */
template<typename NUM_T, typename DENOM_T>
constexpr decltype(auto) div_ceil(NUM_T num, DENOM_T denom) noexcept
{
    // ik, not 'bit math' but this is probably the best place to put this so far
    static_assert(std::is_integral_v<NUM_T>);
    static_assert(std::is_integral_v<DENOM_T>);

    return (num / denom) + (num % denom != 0);
}

template<typename INT_T>
constexpr bool bit_test(INT_T block, int bit) noexcept
{
    static_assert(std::is_unsigned_v<INT_T>);
    return (block & (INT_T(0x1) << bit) ) != 0x0;
}

/**
 * @brief Copy a certain number of bits from one int array to another
 *
 * This function treats integer arrays as bit arrays. Bits are indexed from
 * LSB to MSB.
 *
 * TODO: maybe add a starting bit offset too?
 *
 * @param pSrc   [in] Integer array to copy
 * @param pDest  [out] Integer array to write into
 * @param bits   [in] Number of bits to write
 */
template<typename INT_T>
constexpr void copy_bits(INT_T const* pSrc, INT_T* pDest, std::size_t bits) noexcept
{
    static_assert(std::is_unsigned_v<INT_T>);

    while (bits >= sizeof(INT_T) * 8)
    {
        *pDest = *pSrc;
        pSrc ++;
        pDest ++;
        bits -= sizeof(INT_T) * 8;
    }

    if (bits != 0)
    {
        INT_T const mask = (~INT_T(0x0)) << bits;

        *pDest &= mask;
        *pDest |= ( (~mask) & (*pSrc) );
    }
}

template<typename INT_T>
constexpr void set_bits(INT_T* pDest, std::size_t bits) noexcept
{
    static_assert(std::is_unsigned_v<INT_T>);

    constexpr INT_T c_allOnes = ~INT_T(0x0);

    while (bits >= sizeof(INT_T) * 8)
    {
        *pDest = c_allOnes;
        pDest ++;
        bits -= sizeof(INT_T) * 8;
    }

    if (bits != 0x0)
    {
        *pDest |= ~( c_allOnes << bits );
    }
}

/**
 * @brief Get index of the first set bit found after bit n, returns 0 otherwise
 */
template<typename INT_T>
constexpr int next_bit(INT_T block, int bit) noexcept
{
    static_assert(std::is_unsigned_v<INT_T>);

    INT_T const mask = (~INT_T(0x1)) << bit;
    INT_T const masked = block & mask;

    return (masked == 0) ? 0 : ctz(masked);
}

} // namespace lgrn
