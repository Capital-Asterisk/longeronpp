/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "null.hpp"

#include "../containers/bit_view.hpp"

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
template<typename RANGE_T, typename ID_T>
class BitViewIdRegistry : private BitView<RANGE_T>
{
public:

    using id_int_t      = underlying_int_type_t<ID_T>;
    using base_t        = BitView<RANGE_T>;

    using IdIterator_t  = BitPosIterator<typename base_t::iter_t, typename base_t::sntl_t, false, ID_T>;


    BitViewIdRegistry() = default;
    BitViewIdRegistry(base_t bitview) : base_t(bitview) {};
    BitViewIdRegistry(base_t bitview, [[maybe_unused]] ID_T) : base_t(bitview) {};

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
    constexpr std::size_t capacity() const noexcept { return base_t::size(); }

    /**
     * @return Current number of registered IDs, determined by counting bits
     */
    constexpr std::size_t size() const { return capacity() - base_t::count(); }

    /**
     * @brief Remove an ID. This will mark it for reuse
     */
    void remove(ID_T id) noexcept
    {
        LGRN_ASSERTMV(exists(id), "ID does not exist", std::size_t(id));
        base_t::set(id_int_t(id));
    }

    /**
     * @brief Check if an ID exists
     */
    bool exists(ID_T id) const noexcept
    {
        return (id_int_t(id) < capacity()) && ( ! base_t::test(id_int_t(id)) );
    };

    /**
     * @brief Iterate all existing IDs. Read-only
     */
    constexpr IdIterator_t begin() const noexcept
    {
        return IdIterator_t(std::begin(bitview().ints()), std::end(bitview().ints()), 0, 0);
    }

    constexpr typename IdIterator_t::Sentinel end() const noexcept { return {}; }

    constexpr base_t&       bitview()       noexcept { return static_cast<base_t&>(*this); }
    constexpr base_t const& bitview() const noexcept { return static_cast<base_t const&>(*this); }
};

template<typename RANGE_T, typename ID_T>
template<typename ITER_T, typename SNTL_T>
ITER_T BitViewIdRegistry<RANGE_T, ID_T>::create(ITER_T first, SNTL_T last)
{
    auto const &ones     = base_t::ones();
    auto       onesFirst = ones.begin();
    auto const &onesLast = ones.end();

    while ( (first != last) && (onesFirst != onesLast) )
    {
        std::size_t const pos = *onesFirst;
        *first = ID_T(pos);

        base_t::reset(pos);

        std::advance(onesFirst, 1);
        std::advance(first, 1);
    }
    return first;
}

template <typename IT_T, typename ITB_T, typename ID_T>
constexpr auto bitview_id_reg(IT_T first, ITB_T last, [[maybe_unused]] ID_T id)
{
    return BitViewIdRegistry(lgrn::bit_view(first, last), id);
}

template <typename RANGE_T, typename ID_T>
constexpr auto bitview_id_reg(RANGE_T& rRange, [[maybe_unused]] ID_T id = ID_T(0))
{
    return bitview_id_reg(std::begin(rRange), std::end(rRange), ID_T(0));
}

} // namespace lgrn
