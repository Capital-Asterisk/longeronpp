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

template<typename BITVIEW_T, typename ID_T>
class BitViewIdRegistry : private BITVIEW_T
{
    using id_int_t = underlying_int_type_t<ID_T>;

public:
    BitViewIdRegistry() = default;
    BitViewIdRegistry(BITVIEW_T bitview) : BITVIEW_T(bitview) {};
    BitViewIdRegistry(BITVIEW_T bitview, [[maybe_unused]] ID_T) : BITVIEW_T(bitview) {};

    /**
     * @brief Create a single ID
     *
     * @return A newly created ID
     */
    [[nodiscard]] ID_T create()
    {
        ID_T output{ id_null<ID_T>() };
        create(&output, &output + 1);
        return output;
    }

    /**
     * @brief Create multiple IDs
     *
     * @param first [out]
     * @param last  [in]
     */
    template<typename IT_T, typename ITB_T>
    IT_T create(IT_T first, ITB_T last);

    /**
     * @return Size required to fit all existing IDs, or allocated size if
     *         reserved ahead of time
     */
    constexpr std::size_t capacity() const noexcept { return BITVIEW_T::size(); }

    /**
     * @return Number of registered IDs
     */
    constexpr std::size_t size() const { return capacity() - BITVIEW_T::count(); }

    /**
     * @brief Remove an ID. This will mark it for reuse
     *
     * @param id [in] ID to remove
     */
    void remove(ID_T id)
    {
        LGRN_ASSERTMV(exists(id), "ID does not exist", std::size_t(id));

        BITVIEW_T::set(id_int_t(id));
    }

    /**
     * @brief Check if an ID exists
     *
     * @param id [in] ID to check
     *
     * @return true if ID exists
     */
    bool exists(ID_T id) const noexcept
    {
        return (id_int_t(id) < capacity()) && ( ! BITVIEW_T::test(id_int_t(id)));
    };

    constexpr BITVIEW_T& bitview() noexcept { return static_cast<BITVIEW_T&>(*this); }
    constexpr BITVIEW_T const& bitview() const noexcept { return static_cast<BITVIEW_T const&>(*this); }
};

template<typename BITVIEW_T, typename ID_T>
template<typename IT_T, typename ITB_T>
IT_T BitViewIdRegistry<BITVIEW_T, ID_T>::create(IT_T first, ITB_T last)
{
    auto ones = BITVIEW_T::ones();
    auto onesFirst = std::begin(ones);
    auto onesLast = std::end(ones);

    while ( (first != last) && (onesFirst != onesLast) )
    {
        std::size_t const pos = *onesFirst;
        *first = ID_T(pos);

        BITVIEW_T::reset(pos);

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
