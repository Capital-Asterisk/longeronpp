/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "../utility/asserts.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#ifdef __cpp_lib_span
#include <span>
#endif

namespace lgrn
{

template<typename INT_T, typename SIZE_T, typename PARTITION_SIZE_T>
class PartitionUtils
{
public:

    static constexpr INT_T const smc_null = ~INT_T(0);

    struct DataSpan
    {
        SIZE_T m_offset;
        PARTITION_SIZE_T m_size;
    };

    struct Free
    {
        SIZE_T m_offset{0};
        INT_T m_partitionNum{0};
        INT_T m_partitionCount{0};
        PARTITION_SIZE_T m_size{0};
    };

    struct NewPartition
    {
        SIZE_T m_offset;
        INT_T m_partitionNum;
    };

    static NewPartition create_partition(
            PARTITION_SIZE_T prtnSize,
            Free& rLastFree) noexcept
    {
        // Check remaining space
        if (rLastFree.m_size < prtnSize)
        {
            return NewPartition{ 0, smc_null }; // no space available
        }

        // New partition located on the left of free partition
        SIZE_T const newOffset = rLastFree.m_offset;
        INT_T const newNum = rLastFree.m_partitionNum;

        // Free partition shrinks and shifts right, leaving space on left
        rLastFree.m_size -= prtnSize;
        rLastFree.m_offset += prtnSize;
        rLastFree.m_partitionNum += 1;

        return NewPartition{ newOffset, newNum };
    }

}; // class Partition

template<typename INT_T, typename SIZE_T, typename PARTITION_SIZE_T>
struct PartitionDescStl
{
    using Utils_t           = PartitionUtils<INT_T, SIZE_T, PARTITION_SIZE_T>;
    using NewPartition_t    = typename Utils_t::NewPartition;
    using DataSpan_t        = typename Utils_t::DataSpan;
    using Free_t            = typename Utils_t::Free;

    static constexpr INT_T const smc_null = Utils_t::smc_null;

    void resize(INT_T maxIds)
    {
        m_idToData.resize(maxIds);
        m_idToPartition.resize(maxIds, smc_null);

        m_partitionToId.resize(maxIds, smc_null);
        m_free.reserve(maxIds / 2);
    }

    bool id_in_range(INT_T id) const noexcept { return id < m_idToPartition.size(); }

    typename Utils_t::Free& last_free()
    {
        return m_freeLast;
    }

    constexpr std::size_t count() const noexcept { return m_idCount; }
    constexpr std::size_t used() const noexcept { return m_dataUsed; }

    NewPartition_t create(INT_T id, PARTITION_SIZE_T size)
    {
        NewPartition_t prtn = Utils_t::create_partition(size, m_freeLast);
        m_partitionToId[prtn.m_partitionNum] = id;
        m_idToPartition[id] = prtn.m_partitionNum;
        DataSpan_t &rSpan = m_idToData[id];
        rSpan.m_offset = prtn.m_offset;
        rSpan.m_size = size;

        m_idCount ++;
        m_dataUsed += size;
        return prtn;
    }

    Free_t erase(INT_T id)
    {
        LGRN_ASSERTM(exists(id), "");

        // get partition number
        INT_T const partition = std::exchange(m_idToPartition[id], smc_null);
        m_partitionToId[partition] = smc_null;
        DataSpan_t const data = m_idToData[id];

        Free_t free{data.m_offset, partition, 1, data.m_size};

        if (m_free.empty())
        {
            m_free.push_back(std::move(free));
        }
        else
        {
            auto it = std::upper_bound(
                    std::begin(m_free), std::end(m_free), free,
            [] (Free_t const& lhs, Free_t const& rhs) -> bool {
                return lhs.m_partitionNum > rhs.m_partitionNum;
            });
            m_free.insert(it, std::move(free));
        }

        m_dataUsed -= data.m_size;
        m_idCount --;
        return free;
    }

    struct DataMoved
    {
        SIZE_T m_offsetSrc;
        SIZE_T m_offsetDst;
        SIZE_T m_size;
    };

    /**
     * @return
     */
    DataMoved pack_step(SIZE_T maxMovesHint) noexcept
    {
        if (m_free.empty())
        {
            return {0, 0, 0};
        }
        Free_t &rFirst = m_free.back();
        Free_t &rNext = (m_free.size() == 1) ? m_freeLast : m_free[m_free.size() - 2];

        // strategy: shift partitions between rFirst and rNext left to replace
        //           rFirst, merging it into rNext

        SIZE_T const offsetSrc = rFirst.m_offset + rFirst.m_size;
        SIZE_T const offsetDst = rFirst.m_offset;

        SIZE_T movedData = 0;
        SIZE_T movedPrtn = 0;
        INT_T currentPrtn = rFirst.m_partitionNum;

        while (true)
        {
            INT_T const nextPrtn = currentPrtn + rFirst.m_partitionCount;
            INT_T const nextId = (nextPrtn < m_partitionToId.size())
                               ? m_partitionToId[nextPrtn] : smc_null;

            if (nextId != smc_null)
            {
                // move next partition left to replace current free partition
                m_partitionToId[currentPrtn] = nextId;
                m_partitionToId[nextPrtn] = smc_null;

                m_idToPartition[nextId] = currentPrtn;
                m_idToData[nextId].m_offset -= rFirst.m_size;

                movedPrtn ++;
                movedData += m_idToData[nextId].m_size;

                currentPrtn ++;
            }
            else
            {
                // hit next free partition, merge rFirst into rNext
                LGRN_ASSERT(nextPrtn == rNext.m_partitionNum);
                rNext.m_offset          -= rFirst.m_size;
                rNext.m_partitionNum    -= rFirst.m_partitionCount;
                rNext.m_partitionCount  += rFirst.m_partitionCount;
                rNext.m_size            += rFirst.m_size;

                m_free.pop_back(); // invalidates rFirst
                break;
            }

            if (movedData > maxMovesHint)
            {
                break; // maximum moves exceeded
            }
        }

        return {offsetSrc, offsetDst, movedData};
    }

    bool exists(INT_T id) const noexcept
    {
        return m_idToPartition[id] != smc_null;
    }

    std::vector<INT_T>          m_partitionToId;

    Free_t                      m_freeLast;
    std::vector<Free_t>         m_free;
    std::size_t                 m_dataUsed{0};
    std::size_t                 m_idCount{0};

    std::vector<INT_T>          m_idToPartition;
    std::vector<DataSpan_t>     m_idToData;
};


#ifdef __cpp_lib_span
template<typename ... ARGS_T>
using Span = std::span<ARGS_T...>;
#else
// TODO: use std::span if C++20 is enabled
template <typename DATA_T>
class Span
{
public:
    Span(DATA_T* data, std::size_t size)
     : m_data(data)
     , m_size(size)
    { }

    constexpr std::size_t size() const noexcept { return m_size; }
    constexpr DATA_T& operator[](std::size_t i) const noexcept { return m_data[i]; }
    constexpr DATA_T* begin() const noexcept { return m_data; }
    constexpr DATA_T* end() const noexcept { return m_data + m_size; }

private:
    DATA_T* m_data;
    std::size_t m_size;
};
#endif // __cpp_lib_span

template< typename INT_T, typename DATA_T,
          typename ALLOC_T = std::allocator<DATA_T> >
class IntArrayMultiMap
{
    using alloc_traits_t    = std::allocator_traits<ALLOC_T>;
    using partition_size_t  = unsigned short;

    using PartitionDesc_t   = PartitionDescStl<INT_T, std::size_t, partition_size_t>;
    using Utils_t           = PartitionUtils<INT_T, std::size_t, partition_size_t>;

    using NewPartition_t    = typename Utils_t::NewPartition;
    using Free_t            = typename Utils_t::Free;

    using DataSpan_t        = typename Utils_t::DataSpan;
    using DataMoved_t       = typename PartitionDesc_t::DataMoved;

public:

    IntArrayMultiMap() = default;

    IntArrayMultiMap(INT_T dataCapacity, INT_T idCapacity)
    {
        data_reserve(dataCapacity);
        ids_reserve(idCapacity);
    }

    IntArrayMultiMap(IntArrayMultiMap const& copy) = delete; // TODO

    IntArrayMultiMap(IntArrayMultiMap&& move)
        : m_partitions{std::move(move.m_partitions)}
        , m_allocator{std::move(move.m_allocator)}
        , m_data{std::exchange(move.m_data, nullptr)}
        , m_dataSize{std::exchange(move.m_dataSize, 0)}
    { }

    ~IntArrayMultiMap()
    {
        if (nullptr != m_data)
        {
            // call destructors
            for (INT_T prtnRead = 0; prtnRead < m_partitions.last_free().m_partitionNum; prtnRead ++)
            {
                INT_T const id = m_partitions.m_partitionToId[prtnRead];
                if (id != PartitionDesc_t::smc_null)
                {
                    DataSpan_t const& span = m_partitions.m_idToData[id];
                    std::destroy_n(&m_data[span.m_offset], span.m_size);
                }
            }

            alloc_traits_t::deallocate(m_allocator, m_data, m_dataSize);
        }
    }

    bool contains(INT_T id) const
    {
        if (! m_partitions.id_in_range(id))
        {
            return false;
        }
        return m_partitions.exists(id);
    }

    std::size_t ids_capacity() const noexcept
    {
        return m_partitions.m_idToData.size();
    }

    std::size_t ids_count() const noexcept
    {
        return m_partitions.count();
    }

    void ids_reserve(INT_T capacity)
    {
        m_partitions.resize(capacity);
    }

    constexpr std::size_t data_capacity() const noexcept
    {
        return m_dataSize;
    }

    constexpr std::size_t data_size() const noexcept
    {
        return m_partitions.used();
    }

    void data_reserve(INT_T capacity)
    {
        DATA_T *newData = alloc_traits_t::allocate(m_allocator, capacity);

        Free_t &rLastFree = m_partitions.last_free();

        if (m_data != nullptr)
        {
            // Move all existing partitions into the newly allocated space
            // Additionally, this also removes fragmentation

            INT_T prtnWrite = 0;
            std::size_t writeOffset = 0;

            for (INT_T prtnRead = 0; prtnRead < rLastFree.m_partitionNum; prtnRead ++)
            {
                INT_T const id = m_partitions.m_partitionToId[prtnRead];

                // null partitions are free space
                if (id != PartitionDesc_t::smc_null)
                {
                    DataSpan_t &rSpan = m_partitions.m_idToData[id];
                    std::size_t const offset = rSpan.m_offset;
                    partition_size_t const size = rSpan.m_size;

                    // Make sure partitions fit in new space
                    LGRN_ASSERT(writeOffset <= capacity);

                    std::uninitialized_move_n(
                            &m_data[offset], size, &newData[writeOffset]);

                    if (prtnWrite != prtnRead)
                    {
                        m_partitions.m_partitionToId[prtnRead] = PartitionDesc_t::smc_null;
                        m_partitions.m_partitionToId[prtnWrite] = id;
                        m_partitions.m_idToPartition[id] = prtnWrite;
                        rSpan.m_offset = writeOffset;
                    }

                    writeOffset += size;
                    prtnWrite ++;
                }
            }

            m_partitions.m_free.clear();
            rLastFree.m_offset = writeOffset;
            rLastFree.m_size = capacity - writeOffset;

        }
        else
        {
            rLastFree.m_offset = 0;
            rLastFree.m_size = capacity;
        }

        alloc_traits_t::deallocate(
                m_allocator,
                std::exchange(m_data, newData),
                std::exchange(m_dataSize, capacity));

    }

    template<typename IT_T>
    DATA_T* emplace(INT_T id, IT_T first, IT_T last)
    {
        LGRN_ASSERTMV(m_partitions.id_in_range(id), "ID out of range", id, ids_capacity());
        LGRN_ASSERTMV(!m_partitions.exists(id), "ID does not exist", id);
        partition_size_t size = std::distance(first, last);
        DATA_T* data = create_uninitialized(id, size);
        std::uninitialized_move(first, last, data);
        return data;
    }

    DATA_T* emplace(INT_T id, partition_size_t size)
    {
        LGRN_ASSERTMV(m_partitions.id_in_range(id), "ID out of range", id, ids_capacity());
        LGRN_ASSERTMV(!m_partitions.exists(id), "ID does not exist", id);
        DATA_T* data = create_uninitialized(id, size);
        std::uninitialized_default_construct_n(data, size);
        return data;
    }

    DATA_T* emplace(INT_T id, std::initializer_list<DATA_T> list)
    {
        return emplace(id, std::begin(list), std::end(list));
    }

    void pack(std::size_t maxMoveHint = ~std::size_t(0))
    {
        std::size_t moveTotal = 0;

        while ( !m_partitions.m_free.empty() && (moveTotal < maxMoveHint) )
        {
            DataMoved_t const moved
                    = m_partitions.pack_step(maxMoveHint - moveTotal);

            if (moved.m_size != 0)
            {
                // move some data
                DATA_T *pRead = &m_data[moved.m_offsetSrc];
                DATA_T *pWrite = &m_data[moved.m_offsetDst];

                for (size_t i = 0; i < moved.m_size; i++)
                {
                    ::new(pWrite)DATA_T(std::move(*pRead));

                    pRead ++;
                    pWrite ++;
                }

                moveTotal += moved.m_size;
            }
        }


    }

    void erase(INT_T id)
    {
        Free_t free = m_partitions.erase(id);
        std::destroy_n(&m_data[free.m_offset], free.m_size);
    }

    Span<DATA_T> operator[] (INT_T id) noexcept
    {
        if (!m_partitions.exists(id) || !m_partitions.id_in_range(id))
        {
            return {nullptr, std::size_t(0)};
        }
        DataSpan_t const &span = m_partitions.m_idToData[id];
        return {&m_data[span.m_offset], span.m_size};
    }

    Span<DATA_T const> const operator[] (INT_T id) const noexcept
    {
        if (!m_partitions.exists(id) || !m_partitions.id_in_range(id))
        {
            return {nullptr, std::size_t(0)};
        }
        DataSpan_t const &span = m_partitions.m_idToData[id];
        return {&m_data[span.m_offset], span.m_size};
    }

private:

    DATA_T* create_uninitialized(INT_T id, partition_size_t size)
    {
         NewPartition_t prtn = m_partitions.create(id, size);
         return &m_data[prtn.m_offset];
    }

    PartitionDesc_t m_partitions;
    ALLOC_T m_allocator;
    DATA_T *m_data{nullptr};
    std::size_t m_dataSize{0};

};


}
