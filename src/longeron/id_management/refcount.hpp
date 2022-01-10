/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "storage.hpp"

#include <stdexcept>
#include <vector>

namespace lgrn
{

template<typename COUNT_T = unsigned short>
class RefCount : std::vector<COUNT_T>
{
    using base_t = std::vector<COUNT_T>;
public:

    RefCount() = default;
    RefCount(RefCount&& move) = default;
    RefCount(std::size_t capacity)
     : base_t( capacity, 0 )
    { };

    // Delete copy
    RefCount(RefCount const& copy) = delete;
    RefCount& operator=(RefCount const& copy) = delete;

    // Allow move assign only if all counts are zero
    RefCount& operator=(RefCount&& move)
    {
        assert(only_zeros_remaining(0));
        base_t::operator=(std::move(move));
        return *this;
    }

    ~RefCount()
    {
        // Make sure ref counts are all zero on destruction
        assert(only_zeros_remaining(0));
    }

    bool only_zeros_remaining(std::size_t start) const noexcept
    {
        for(auto it = std::next(begin(), start); it != end(); std::advance(it))
        {
            if (0 != *it)
            {
                return false;
            }
        }
        return true;
    }

    using base_t::size;
    using base_t::operator[];

    void resize(std::size_t newSize)
    {
        if (newSize < size())
        {
            // sizing down, make sure zeros
            if ( ! only_zeros_remaining(newSize))
            {
                throw std::runtime_error("Downsizing non-zero ref counts");
            }
        }
        base_t::resize(newSize, 0);
    }

}; // class RefCount

template<typename ID_T, typename COUNT_T = unsigned short>
class IdRefCount : public RefCount<COUNT_T>
{
    using id_int_t = underlying_int_type_t<ID_T>;

public:

    using Storage_t = IdStorage<ID_T, IdRefCount>;

    Storage_t ref_add(ID_T id)
    {
        auto const idInt = id_int_t(id);
        if (this->size() <= idInt)
        {
            this->resize(idInt + 1);
        }
        (*this)[idInt] ++;

        return Storage_t(id);
    }

    void ref_release(Storage_t& rStorage) noexcept
    {
        if (rStorage.has_value())
        {
            auto const idInt = id_int_t(rStorage.m_id);
            (*this)[idInt] --;
            rStorage.m_id = id_null<ID_T>();
        }
    }

}; // class IdRefCount

} // namespace lgrn
