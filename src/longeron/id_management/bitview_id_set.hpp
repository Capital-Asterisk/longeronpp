/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "../containers/bit_view.hpp"

#include "../utility/enum_traits.hpp"


namespace lgrn
{

/**
 * @brief Wraps a BitView to be used as a set of IDs. The interface similar to std::set<ID_T>.
 */
template<typename RANGE_T, typename ID_T, bool ONES = true>
class BitViewIdSet : private BitView<RANGE_T>
{
public:

    using id_int_t      = underlying_int_type_t<ID_T>;
    using base_t        = BitView<RANGE_T>;

    using IdIterator_t  = BitPosIterator<typename base_t::iter_t, typename base_t::sntl_t, ONES, ID_T>;

    using base_t::base_t;
    using base_t::operator=;

    constexpr base_t&       bitview()       noexcept { return static_cast<base_t&>(*this); }
    constexpr base_t const& bitview() const noexcept { return static_cast<base_t const&>(*this); }

    // Capacity

    /**
     * @return Max number of IDs that can be stored
     */
    constexpr std::size_t capacity() const noexcept { return base_t::size(); }

    /**
     * @return Current number of contained IDs, determined by counting bits
     */
    constexpr std::size_t size() const
    {
        if constexpr (ONES)
        {
            return base_t::count();
        }
        else
        {
            return capacity() - base_t::count();
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
    constexpr IdIterator_t begin() const noexcept
    {
        return IdIterator_t(std::begin(bitview().ints()), std::end(bitview().ints()), 0, 0);
    }

    constexpr typename IdIterator_t::Sentinel end() const noexcept { return {}; }

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

    void clear()
    {
        if constexpr (ONES)
        {
            base_t::reset();
        }
        else
        {
            base_t::set();
        }
    }

private:

    static constexpr id_int_t smc_emptyInt = ONES ? id_int_t(0) : ~id_int_t(0);

    bool impl_contains(std::size_t const pos) const
    {
        if constexpr (ONES)
        {
            return base_t::test(pos);
        }
        else
        {
            return ! base_t::test(pos);
        }
    }

    void impl_insert(std::size_t const pos)
    {
        if constexpr (ONES)
        {
            base_t::set(pos);
        }
        else
        {
            base_t::reset(pos);
        }
    }

    void impl_erase(std::size_t const pos)
    {
        if constexpr (ONES)
        {
            base_t::reset(pos);
        }
        else
        {
            base_t::clear(pos);
        }
    }
};

} // namespace lgrn
