/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "cast_iterator.hpp"
#include "../utility/enum_traits.hpp"

#include <algorithm>

namespace lgrn
{

/**
 * @brief Wraps a BitView to be used as a set of IDs. The interface similar to std::set<ID_T>.
 *
 * @warning Untested when ONES = false
 */
template<typename BITVIEW_T, typename ID_T, bool ONES = true>
class BitViewIdSet : private BITVIEW_T
{
    using id_int_t      = underlying_int_type_t<ID_T>;
    using IterBase_t    = std::conditional_t<ONES, typename BITVIEW_T::OnesIter_t,
                                                   typename BITVIEW_T::ZerosIter_t>;
    using SntlBase_t    = std::conditional_t<ONES, typename BITVIEW_T::OnesSntl_t,
                                                   typename BITVIEW_T::ZerosSntl_t>;
public:

    using Base_t        = BITVIEW_T;
    using Iterator_t    = IdCastIterator<IterBase_t, ID_T>;
    using Sentinel_t    = SntlBase_t;

    using Base_t::Base_t;
    using Base_t::operator=;

    constexpr Base_t&       bitview()       noexcept { return static_cast<Base_t&>(*this); }
    constexpr Base_t const& bitview() const noexcept { return static_cast<Base_t const&>(*this); }

    // Capacity

    /**
     * @return Max number of IDs that can be stored
     */
    constexpr std::size_t capacity() const noexcept { return Base_t::size(); }

    /**
     * @return Current number of contained IDs, determined by counting bits
     */
    constexpr std::size_t size() const
    {
        if constexpr (ONES)
        {
            return Base_t::count();
        }
        else
        {
            return capacity() - Base_t::count();
        }
    }

    bool empty() const noexcept
    {
        return std::all_of(std::begin(bitview().ints()), std::end(bitview().ints()),
                           [] ( id_int_t const value )  { return value == smc_emptyInt; });
    }

    // Iterators

    /**
     * @brief Iterate all existing IDs. Read-only
     */
    constexpr Iterator_t begin() const noexcept
    {
        if constexpr (ONES)
        {
            return Iterator_t{ bitview().ones().begin() };
        }
        else
        {
            return Iterator_t{ bitview().zeros().begin() };
        }
    }

    constexpr Sentinel_t end() const noexcept
    {
        if constexpr (ONES)
        {
            return bitview().ones().end();
        }
        else
        {
            return bitview().zeros().end();
        }
    }

    // Element Lookup

    std::size_t count(ID_T id) const noexcept
    {
        return std::size_t(impl_contains(std::size_t(id)));
    }

    bool contains(ID_T id) const noexcept
    {
        return impl_contains(std::size_t(id));
    }

    // Modifiers

    struct InsertResult
    {
        std::size_t first; ///< Position
        bool second; ///< Insertion succeeded
    };

    template<typename ... ARGS_T>
    InsertResult emplace(ARGS_T &&... args)
    {
        // this feels pretty stupid
        return insert(ID_T{ std::forward<ARGS_T>(args)... });
    }

    InsertResult insert(ID_T id)
    {
        auto const idInt = std::size_t(id);

        bool const exists = impl_contains(idInt);
        impl_insert(idInt);

        return {idInt, ! exists};
    }

    template <typename ITER_T, typename SNTL_T>
    void insert(ITER_T first, SNTL_T const last)
    {
        while (first != last)
        {
            impl_insert(  std::size_t( ID_T{*first} )  );
            ++first;
        }
    }

    void insert(std::initializer_list<ID_T> list)
    {
        return insert(list.begin(), list.end());
    }

    std::size_t erase(ID_T id)
    {
        auto const idInt  = std::size_t(id);
        bool const exists = impl_contains(idInt);

        impl_erase(idInt);
        return std::size_t(exists);
    }

    template <typename ITER_T, typename SNTL_T>
    void erase(ITER_T first, SNTL_T const last)
    {
        while (first != last)
        {
            impl_erase(  std::size_t{ ID_T{*first} }  );
            ++first;
        }
    }

    void clear() noexcept
    {
        if constexpr (ONES) { Base_t::reset(); } else { Base_t::set(); }
    }

private:

    static constexpr id_int_t smc_emptyInt = ONES ? id_int_t(0) : ~id_int_t(0);

    bool impl_contains(std::size_t const pos) const noexcept
    {
        if constexpr (ONES) { return Base_t::test(pos); } else { return ! Base_t::test(pos); }
    }

    void impl_insert(std::size_t const pos) noexcept
    {
        if constexpr (ONES) { Base_t::set(pos); } else { Base_t::reset(pos); }
    }

    void impl_erase(std::size_t const pos) noexcept
    {
        if constexpr (ONES) { Base_t::reset(pos); } else { Base_t::clear(pos); }
    }
};

} // namespace lgrn
