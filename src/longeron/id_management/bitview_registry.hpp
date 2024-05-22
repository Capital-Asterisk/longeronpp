/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "cast_iterator.hpp"
#include "null.hpp"

#include "../utility/enum_traits.hpp"
#include "../utility/asserts.hpp"

namespace lgrn
{

/**
 * @brief Adapts a bitview to an interface for creating/destroying unique integer IDs
 *
 * Ones are used as free IDs, and zeros are used as taken. This is because the bitwise operations
 * used are slightly faster at searching for ones.
 */
template<typename BITVIEW_T, typename ID_T>
class BitViewIdRegistry : private BITVIEW_T
{
    using id_int_t      = underlying_int_type_t<ID_T>;

public:

    using Base_t        = BITVIEW_T;
    using Iterator_t    = IdCastIterator<typename Base_t::ZerosIter_t, ID_T>;
    using Sentinel_t    = typename Base_t::ZerosSntl_t;

    constexpr Base_t&       bitview()       noexcept { return static_cast<Base_t&>(*this); }
    constexpr Base_t const& bitview() const noexcept { return static_cast<Base_t const&>(*this); }

    BitViewIdRegistry() = default;
    BitViewIdRegistry(Base_t bitview) : Base_t(bitview) {};
    BitViewIdRegistry(Base_t bitview, [[maybe_unused]] ID_T) : Base_t(bitview) {};

    /**
     * @brief Create a single ID
     */
    [[nodiscard]] ID_T create()
    {
        ID_T output{ id_null<ID_T>() };
        create(&output, &output + 1);
        return output;
    }

    /**
     * @brief Create multiple IDs, store new IDs in a specified range
     *
     * The number of IDs created will be limited by available space for IDs and size of the range.
     */
    template<typename ITER_T, typename SNTL_T>
    ITER_T create(ITER_T first, SNTL_T last);

    /**
     * @return Max number of IDs that can be stored
     */
    constexpr std::size_t capacity() const noexcept { return Base_t::size(); }

    /**
     * @return Current number of registered IDs, determined by counting bits
     */
    constexpr std::size_t size() const { return capacity() - Base_t::count(); }

    /**
     * @brief Remove an ID. This will mark it for reuse
     */
    void remove(ID_T id) noexcept
    {
        LGRN_ASSERTMV(exists(id), "ID does not exist", std::size_t(id));
        Base_t::set(id_int_t(id));
    }

    /**
     * @brief Check if an ID exists
     */
    bool exists(ID_T id) const noexcept
    {
        return (id_int_t(id) < capacity()) && ( ! Base_t::test(id_int_t(id)) );
    };

    /**
     * @brief Iterate all existing IDs. Read-only
     */
    constexpr Iterator_t begin() const noexcept
    {
        return Iterator_t{ bitview().zeros().begin() };
    }

    constexpr Sentinel_t end() const noexcept { return {}; }

};

template<typename BITVIEW_T, typename ID_T>
template<typename ITER_T, typename SNTL_T>
ITER_T BitViewIdRegistry<BITVIEW_T, ID_T>::create(ITER_T first, SNTL_T last)
{
    auto const &ones     = Base_t::ones();
    auto       onesFirst = ones.begin();
    auto const &onesLast = ones.end();

    while ( (first != last) && (onesFirst != onesLast) )
    {
        std::size_t const pos = *onesFirst;
        *first = ID_T(pos);

        Base_t::reset(pos);

        std::advance(onesFirst, 1);
        std::advance(first, 1);
    }
    return first;
}

//template <typename IT_T, typename ITB_T, typename ID_T>
//constexpr auto bitview_id_reg(IT_T first, ITB_T last, [[maybe_unused]] ID_T id)
//{
//    return BitViewIdRegistry(lgrn::bit_view(first, last), id);
//}

//template <typename BITVIEW_T, typename ID_T>
//constexpr auto bitview_id_reg(BITVIEW_T& rRange, [[maybe_unused]] ID_T id = ID_T(0))
//{
//    return bitview_id_reg(std::begin(rRange), std::end(rRange), ID_T(0));
//}

} // namespace lgrn
