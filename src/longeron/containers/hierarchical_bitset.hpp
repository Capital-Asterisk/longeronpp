/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "../utility/bitmath.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace lgrn
{

/**
 * @brief Divide two integers and round up
 */
template<typename NUM_T, typename DENOM_T>
constexpr decltype(auto) div_ceil(NUM_T num, DENOM_T denom) noexcept
{
    static_assert(std::is_integral_v<NUM_T>);
    static_assert(std::is_integral_v<DENOM_T>);

    return (num / denom) + (num % denom != 0);
}

//-----------------------------------------------------------------------------

/**
 * @brief A bitset featuring hierarchical rows for fast iteration over set bits
 *
 * This container requires rows of bits, divided into blocks. This is
 * represented as an array of unsigned integer types, where each int is a block.
 *
 * The main advantage of using an int, is that their bits can be tested for
 * ones all at once, by comparing the int to zero.
 *
 * There are multiple rows. The bottom row [0] is the user's bits that can be
 * set and reset. For the rest of the rows above, each bit N is set if the row
 * below's block N is non-zero.
 *
 * The bottom row sets the size of the container, and each row above must be
 * be large enough to have a bit for each block of the row below.
 *
 * For example, when using 8-bit block sizes, and 40 bits for container size:
 *
 * Row 0 size: 40 bits  (5 blocks needed)
 * Row 1 size: 5 bits   (1 block)
 * Row 2 size: 0, not needed
 * ...
 *
 * To represent a set of integers: { 0, 1, 18, 19 }:
 *
 * Row 1: 00000101
 * Row 0: 00000011 00000000 00001100 00000000 00000000
 *
 * * Row 1's first 5 bits corresponds to Row 0's blocks.
 * * Row 1's bit 0 and bit 2 means Row 0's block 0 and block 2 is non-zero
 *
 * note: Bits are displayed in msb->lsb, but arrays are left->right
 *
 * Iterating or searching for set bits on the bottom row is as simple as looking
 * up the set bits in the blocks the top row, and recursing down.
 *
 * This operation is O(number of rows), or O(log n)
 */
template <typename BLOCK_INT_T = uint64_t>
class HierarchicalBitset
{
    using row_index_t = std::size_t;

    static constexpr int smc_blockSize = sizeof(BLOCK_INT_T) * 8;

    // Max size of the top row.
    // A higher number means functions like take() must search the top level for
    // non-zero blocks, but 1 less row to manage. Unlikely this will do anything
    static constexpr int smc_topLevelMaxBlocks = 1;

    // Max bits representable = smc_blockSize^smc_maxRows
    // Due to its exponential nature, this number doesn't need to be that high
    static constexpr int smc_maxRows = 8;

    struct RowBit
    {
        row_index_t m_block{0};
        int m_bit{-1};

        constexpr bool valid() const noexcept { return m_bit != -1; }
    };

public:

    struct Row { std::size_t m_offset; std::size_t m_size; };
    using rows_t = std::array<Row, smc_maxRows>;

    struct Iterator
    {
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::size_t;
        using pointer           = void;
        using reference         = void;

        Iterator(HierarchicalBitset const* pContainer, RowBit pos)
         : m_pContainer(pContainer)
         , m_pos(pos)
        { }

        constexpr value_type operator*() const noexcept
        {
            return m_pos.m_block * smc_blockSize + m_pos.m_bit;
        }

        Iterator& operator++() noexcept
        {
            RowBit const nextPos = m_pContainer->next(0, m_pos);

            m_pos = nextPos.valid()
                  ? nextPos
                  : m_pContainer->bit_at(m_pContainer->size());

            return *this;
        }

        constexpr friend bool operator>(Iterator const& lhs, Iterator const& rhs) noexcept
        {
            return *lhs > *rhs;
        }

        constexpr friend bool operator<(Iterator const& lhs, Iterator const& rhs) noexcept
        {
            return *lhs < *rhs;
        }

        constexpr friend bool operator==(Iterator const& lhs, Iterator const& rhs) noexcept
        {
            return (lhs.m_pContainer == rhs.m_pContainer)
                   && (lhs.m_pos.m_block == rhs.m_pos.m_block)
                   && (lhs.m_pos.m_bit == rhs.m_pos.m_bit);
        };

        constexpr friend bool operator!=(Iterator const& lhs, Iterator const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

    private:

        HierarchicalBitset const *m_pContainer;
        RowBit m_pos;
    };

    constexpr HierarchicalBitset() = default;

    /**
     * @brief Construct with size
     *
     * @param size [in] Size in bits
     * @param fill [in] Initialize with 1 bits if true
     */
    HierarchicalBitset(std::size_t size, bool fill = false)
     : m_size{size}
     , m_topLevel{0}
     , m_blockCount{ calc_blocks_recurse(size, 0, m_topLevel, m_rows) }
     , m_blocks{ std::make_unique<BLOCK_INT_T[]>(m_blockCount) }
    {
        if (fill)
        {
            set();
        }
    }

    std::size_t front() const noexcept
    {
        if (m_size == 0)
        {
            return m_size;
        }
        return test(0) ? 0 : next(0);
    }

    Iterator begin() const { return Iterator(this, bit_at(front())); }

    Iterator end() const { return Iterator(this, bit_at(m_size)); }

    /**
     * @brief Test if a bit is set
     *
     * @param bit [in] Bit number
     *
     * @return True if bit is set, False if not, 3.14159 if the universe broke
     */
    bool test(std::size_t bit) const
    {
        bounds_check(bit);
        RowBit const pos = bit_at(bit);

        return bit_test(m_blocks[m_rows[0].m_offset + pos.m_block], pos.m_bit);
    }

    /**
     * @brief Set all bits to 1
     */
    void set() noexcept
    {
        for (int i = 0; i < m_topLevel + 1; i ++)
        {
            std::size_t const bits = (i != 0) ? m_rows[i - 1].m_size : m_size;
            set_bits(&m_blocks[m_rows[i].m_offset], bits);
        }

        m_count = m_size;
    }

    /**
     * @return Total number of supported bits
     */
    constexpr std::size_t size() const noexcept { return m_size; }

    /**
     * @return Number of set bits
     */
    constexpr std::size_t count() const noexcept { return m_count; }

    /**
     * @brief Set a bit to 1
     *
     * @param bit [in] Bit to set
     */
    void set(std::size_t bit)
    {
        bounds_check(bit);
        block_set_recurse(0, bit_at(bit));
    }

    /**
     * @brief Reset a bit to 0
     *
     * @param bit [in] Bit to reset
     */
    void reset(std::size_t bit)
    {
        bounds_check(bit);
        block_reset_recurse(0, bit_at(bit));
    }

    /**
     * @brief Reset all bits to 0
     */
    void reset() noexcept
    {
        std::fill(m_blocks.get(), m_blocks.get() + m_blockCount, 0);
        m_count = 0;
    }

    /**
     * @brief Get first set bit after a specified bit
     *
     * @param bit [in] Bits after this bit index will be 'searched' for ones
     *
     * @return Bit index of next found bit, end of container if not found.
     */
    std::size_t next(std::size_t bit) const noexcept
    {
        RowBit const nextPos = next(0, bit_at(bit));
        if ( ! nextPos.valid())
        {
            return m_size; // no bit next
        }
        return nextPos.m_block * smc_blockSize + nextPos.m_bit;
    }

    /**
     * @brief Take multiple bits, clear them, and write their indices to an
     *        iterator.
     *
     * @param out           [out] Output Iterator
     * @param count         [in] Number of bits to take
     *
     * @tparam IT_T         Output iterator type
     * @tparam CONVERT_T    Functor or type to convert index to iterator value
     *
     * @return Remainder from count, non-zero if container becomes empty
     */
    template<typename IT_T, typename CONVERT_T = std::size_t>
    std::size_t take(IT_T out, std::size_t count);

    /**
     * @brief Reallocate to fit a certain number of bits
     *
     * @param size [in] Size in bits
     * @param fill [in] If true, new space will be set to 1
     */
    void resize(std::size_t size, bool fill = false);

    // TODO: a highest bit function and iterators

    /**
     * @return Index of top row
     */
    constexpr int top_row() const noexcept { return m_topLevel; }

    /**
     * @return Read-only access to row offsets and sizes
     */
    constexpr rows_t const& rows() const noexcept { return m_rows; }

    /**
     * @return Read-only access to block data
     */
    BLOCK_INT_T const* data() const noexcept { return m_blocks.get(); }

private:

    /**
     * @brief Calculate required number of blocks
     *
     * @param bitCount  [in] Bits needed to be represented by this row
     * @param dataUsed  [in] Current number of blocks used
     * @param rLevel    [out] Current or highest level reached
     * @param rLevels   [out] Row offsets and sizes
     *
     * @return Total number of blocks required
     */
    static constexpr std::size_t calc_blocks_recurse(
            std::size_t bitCount, std::size_t dataUsed, int& rLevel, rows_t& rLevels);

    /**
     * @brief Convert bit position to a row block number and block bit number
     */
    static constexpr RowBit bit_at(std::size_t rowBit) noexcept
    {
        row_index_t const block = rowBit / smc_blockSize;
        int const bit = rowBit % smc_blockSize;
        return { block, bit };
    }

    /**
     * @brief Throw an exception if pos is out of this container's range
     *
     * @param pos [in] Position to check
     */
    constexpr void bounds_check(std::size_t pos) const
    {
        if (pos >= m_size)
        {
            throw std::out_of_range("Bit position out of range");
        }
    }

    /**
     * @brief Set a bit and recursively set bits of rows above if needed
     *
     * @param level [in] Current level (row)
     * @param pos   [in] Bit to set
     */
    void block_set_recurse(int level, RowBit pos) noexcept;

    /**
     * @brief Resets a bit and recursively resets bits of rows above if needed
     *
     * @param level [in] Current level (row)
     * @param pos   [in] Bit to reset
     */
    void block_reset_recurse(int level, RowBit pos) noexcept;

    constexpr RowBit next(int level, RowBit pos) const noexcept
    {
        BLOCK_INT_T const &rBlock = m_blocks[m_rows[level].m_offset + pos.m_block];

        // Try to get next bit on same row
        int const nextBit = next_bit(rBlock, pos.m_bit);

        if (nextBit != 0)
        {
            return {pos.m_block, nextBit}; // Next bit found succesfully
        }

        if (level == m_topLevel)
        {
            return {}; // Top level and no more blocks available
        }

        // Look for next non-zero block
        RowBit const nextUpperPos = next(level + 1, bit_at(pos.m_block));

        if ( ! nextUpperPos.valid())
        {
            return {}; // No more blocks available
        }

        std::size_t const nextBlkOffset
                = nextUpperPos.m_block * smc_blockSize + nextUpperPos.m_bit;

        int const nextBlkBit
                = ctz(m_blocks[m_rows[level].m_offset + nextBlkOffset]);

        // Next block found
        return {nextBlkOffset, nextBlkBit};
    }

    /**
     * @brief Recursive function to walk down the hierarchy, arriving at a set
     *        bit at Row 0
     *
     * @param level     [in] Current Level
     * @param blockNum  [in] Row's block number
     * @param rOut      [out] Output iterator to write indices to
     * @param rCount    [out] Remaining values to write to output iteratr
     *
     * @return true if block is non-zero and/or count stopped
     *         false if block is zero
     */
    template<typename IT_T, typename CONVERT_T>
    constexpr bool take_recurse(
            int level, std::size_t blockNum, IT_T& rOut, std::size_t& rCount);

    /**
     * @brief Recalculate m_count by counting bits
     */
    void recount();

    /**
     * @brief Recalculate bits of all valid rows above 0
     */
    void recalc_blocks();

    rows_t m_rows;
    std::size_t m_size{0};
    std::size_t m_count{0};
    int m_topLevel{0};

    std::size_t m_blockCount{0};
    std::unique_ptr<BLOCK_INT_T[]> m_blocks;

}; // class HierarchicalBitset


template <typename BLOCK_INT_T>
template<typename IT_T, typename CONVERT_T>
std::size_t HierarchicalBitset<BLOCK_INT_T>::take(IT_T out, std::size_t count)
{
    int level = m_topLevel;

    // Loop through top-level blocks
    for (row_index_t i = 0; i < m_rows[m_topLevel].m_size; i ++)
    {
        take_recurse<IT_T, CONVERT_T>(level, i, out, count);

        if (0 == count)
        {
            break;
        }
    }

    return count;
}

template <typename BLOCK_INT_T>
void HierarchicalBitset<BLOCK_INT_T>::resize(std::size_t size, bool fill)
{
    HierarchicalBitset replacement(size);

    if (fill)
    {
        replacement.set();
    }

    // copy row 0 to new replacment
    copy_bits(&m_blocks[m_rows[0].m_offset],
              &replacement.m_blocks[replacement.m_rows[0].m_offset],
              std::min(m_size, replacement.m_size));

    replacement.recalc_blocks();
    replacement.recount();

    *this = std::move(replacement);
}

template <typename BLOCK_INT_T>
constexpr std::size_t HierarchicalBitset<BLOCK_INT_T>::calc_blocks_recurse(
        std::size_t bitCount, std::size_t dataUsed, int& rLevel, rows_t& rLevels)
{
    // blocks needed to fit [bitCount] bits
    std::size_t const blocksRequired = div_ceil(bitCount, smc_blockSize);

    rLevels[rLevel] = { dataUsed, blocksRequired };

    if (blocksRequired > smc_topLevelMaxBlocks)
    {
        rLevel ++;
        return blocksRequired + calc_blocks_recurse(
                blocksRequired, dataUsed + blocksRequired, rLevel, rLevels );
    }

    return blocksRequired;
}

template <typename BLOCK_INT_T>
void HierarchicalBitset<BLOCK_INT_T>::block_set_recurse(
        int level, RowBit pos) noexcept
{
    BLOCK_INT_T &rBlock = m_blocks[m_rows[level].m_offset + pos.m_block];

    BLOCK_INT_T const blockOld = rBlock;
    rBlock |= (BLOCK_INT_T(0x1) << pos.m_bit);

    if ( blockOld != rBlock )
    {
        // something changed
        if (0x0 == level)
        {
            m_count ++;
        }

        if ( blockOld == 0x0 && (level != m_topLevel) )
        {
            // Recurse, as block was previously zero
            block_set_recurse(level + 1, bit_at(pos.m_block));
        }
    }
}

template <typename BLOCK_INT_T>
void HierarchicalBitset<BLOCK_INT_T>::block_reset_recurse(
        int level, RowBit pos) noexcept
{
    BLOCK_INT_T &rBlock = m_blocks[m_rows[level].m_offset + pos.m_block];

    if (rBlock == 0x0)
    {
        return; // block already zero, do nothing
    }

    BLOCK_INT_T const blockOld = rBlock;
    rBlock &= ~(BLOCK_INT_T(1) << pos.m_bit); // Clear bit

    if ( blockOld != rBlock )
    {
        if (0 == level)
        {
            m_count --;
        }

        if ( (rBlock == 0x0) && (level != m_topLevel) )
        {
            // Recurse, as block was just made zero
            block_reset_recurse(level + 1, bit_at(pos.m_block));
        }
    }

}

template <typename BLOCK_INT_T>
template<typename IT_T, typename CONVERT_T>
constexpr bool HierarchicalBitset<BLOCK_INT_T>::take_recurse(
        int level, std::size_t blockNum, IT_T& rOut, std::size_t& rCount)
{
    BLOCK_INT_T &rBlock = m_blocks[m_rows[level].m_offset + blockNum];

    while (0x0 != rBlock)
    {
        // Return if enough bits have been taken
        if (0 == rCount)
        {
            return true;
        }

        int const blockBit = ctz(rBlock);
        std::size_t const rowBit = blockNum * smc_blockSize + blockBit;

        if (0 == level)
        {
            // Take the bit
            *rOut = CONVERT_T(rowBit); // Write row bit number to output
            std::advance(rOut, 1);
            rCount --;
            m_count --;
        }
        else
        {
            // Recurse into row and block below
            if ( take_recurse<IT_T, CONVERT_T>(level - 1, rowBit, rOut, rCount) )
            {
                // Returned true, block below isn't zero, don't clear bit
                continue;
            }
        }

        rBlock &= ~(BLOCK_INT_T(0x1) << blockBit); // Clear bit
    }

    return false; // Block is zero, no more 1 bits left

}

template <typename BLOCK_INT_T>
void HierarchicalBitset<BLOCK_INT_T>::recount()
{
    m_count = 0;
    for (std::size_t i = 0; i < m_rows[0].m_size; i ++)
    {
        m_count += std::bitset<smc_blockSize>(
                    m_blocks[m_rows[0].m_offset + i]).count();
    }
}

template <typename BLOCK_INT_T>
void HierarchicalBitset<BLOCK_INT_T>::recalc_blocks()
{
    for (int i = 0; i < m_topLevel; i ++)
    {
        Row const current = m_rows[i + 1];
        Row const below = m_rows[i];

        for (std::size_t j = 0; j < current.m_size; j ++)
        {
            BLOCK_INT_T blockNew = 0;

            std::size_t const belowBlocks = std::min<std::size_t>(
                        smc_blockSize, below.m_size - j * smc_blockSize);

            BLOCK_INT_T currentBit = 0x1;

            // Evaluate a block for each bit
            for (int k = 0; k < belowBlocks; k ++)
            {
                blockNew |= currentBit * (m_blocks[below.m_offset + k]);
                currentBit <<= 1;
            }

            m_blocks[current.m_offset + j] = blockNew;
        }
    }
}

}
