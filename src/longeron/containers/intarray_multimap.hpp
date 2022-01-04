/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include <utility>
#include <memory>
#include <vector>

#include <cassert>

namespace lgrn
{

template<typename INT_T, typename SIZE_T, typename PARTITION_SIZE_T>
class PartitionUtils
{
public:

    struct Span
    {
        SIZE_T m_offset;
        PARTITION_SIZE_T m_size;
    };

    struct Free
    {
        SIZE_T m_offset{0};
        INT_T m_partitionNum{0};
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
            return NewPartition{ ~SIZE_T(0), ~INT_T(0) }; // no space available
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
    using Utils_t = PartitionUtils<INT_T, SIZE_T, PARTITION_SIZE_T>;
    using Span_t = typename Utils_t::Span;
    using Free_t = typename Utils_t::Free;

    void resize(INT_T maxIds)
    {
        m_idToData.resize(maxIds);
        m_idToPartition.resize(maxIds, ~INT_T(0));
        m_partitionToId.resize(maxIds, ~INT_T(0));
        m_free.reserve(maxIds / 2);
    }

    bool id_in_range(INT_T id) const noexcept { return id < m_idToPartition.size(); }

    typename Utils_t::Free& last_free()
    {
        return m_freeLast;
    }

    void assign_id(
            INT_T id, INT_T partition, PARTITION_SIZE_T size, SIZE_T offset)
    {
        m_partitionToId[partition] = id;
        m_idToPartition[id] = partition;
        Span_t &rSpan = m_idToData[id];
        rSpan.m_offset = offset;
        rSpan.m_size = size;
    }

    void erase(INT_T id)
    {
        assert(exists(id));

        // get partition number
        INT_T const partition = std::exchange(m_idToPartition[id], ~INT_T(0));
        m_partitionToId[id] = ~INT_T(0);
        Span_t const data = m_idToData[id];

        m_free.push_back({data.m_offset, partition, data.m_size});
    }

    bool exists(INT_T id) const noexcept
    {
        return m_idToPartition[id] != ~INT_T(0);
    }

    std::vector<INT_T> m_partitionToId;

    typename Utils_t::Free m_freeLast;
    std::vector<typename Utils_t::Free> m_free;

    std::vector<INT_T> m_idToPartition;
    std::vector<Span_t> m_idToData;
};

template< typename INT_T, typename DATA_T,
          typename ALLOC_T = std::allocator<DATA_T> >
class IntArrayMultiMap
{
    using alloc_traits_t    = std::allocator_traits<ALLOC_T>;
    using partition_size_t  = unsigned short;

    using PartitionDesc_t   = PartitionDescStl<INT_T, std::size_t, partition_size_t>;
    using Utils_t           = PartitionUtils<INT_T, std::size_t, partition_size_t>;

    using NewPartition_t    = typename Utils_t::NewPartition;

public:
    IntArrayMultiMap(INT_T capacity)
    {
        resize(capacity);
    }

    ~IntArrayMultiMap()
    {
        if (nullptr != m_data)
        {
            deallocate();
        }
    }

    void resize_ids(INT_T capacity)
    {
        m_partitions.resize(capacity);
    }

    void resize(INT_T capacity)
    {
        DATA_T *newData = alloc_traits_t::allocate(m_allocator, capacity);

        if (m_data != nullptr)
        {
            // TODO: move
            assert(1);
        }
        m_data = newData;

        m_partitions.last_free().m_size += capacity - m_dataSize;
        m_dataSize = capacity;
    }

    template<typename IT_T>
    DATA_T* emplace(INT_T id, IT_T first, IT_T last)
    {
        assert(m_partitions.id_in_range(id));
        assert(!m_partitions.exists(id));
        partition_size_t size = std::distance(first, last);
        DATA_T* data = create_uninitialized(id, size);
        std::uninitialized_move(first, last, data);
        return data;
    }

    void erase(INT_T id)
    {
        m_partitions.erase(id);
    }

private:

    DATA_T* create_uninitialized(INT_T id, partition_size_t size)
    {
         NewPartition_t prtn
                = Utils_t::create_partition(size, m_partitions.last_free());
         m_partitions.assign_id(id, prtn.m_partitionNum, size, prtn.m_offset);
         return &m_data[prtn.m_offset];
    }

    void deallocate()
    {
        alloc_traits_t::deallocate(m_allocator, std::exchange(m_data, nullptr), m_dataSize);
    }

    PartitionDesc_t m_partitions;
    ALLOC_T m_allocator;
    DATA_T *m_data{nullptr};
    std::size_t m_dataSize{0};

};


}
