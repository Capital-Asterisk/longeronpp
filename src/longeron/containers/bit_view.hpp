/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "bit_iterator.hpp"

#include <type_traits>

namespace lgrn
{

// somewhat of a re-invented std::views::filter
template <typename IT_T, typename ITB_T, bool(*SKIP_FUNC)(IT_T const&)>
class SkipValueIterator : public IT_T
{
public:
    SkipValueIterator(IT_T current, IT_T end)
     : IT_T(current)
     , m_end(end)
    { }
    constexpr SkipValueIterator& operator++() noexcept
    {
        do
        {
            IT_T::operator++();
        }
        while (*this != m_end && SKIP_FUNC(*this));

        return *this;
    }
private:
    ITB_T m_end;
};


/**
 * @brief Adapts a bit interface around an integer span-like type
 *
 * Not intended for storage purposes.
 */
template <typename IT_T, typename ITB_T>
class BitView
{
    using int_t = typename std::iterator_traits<IT_T>::value_type;

    static_assert(std::is_unsigned_v<int_t>, "Use only unsigned types for bit manipulation");

    template<int_t VALUE>
    static bool stupid_test(IT_T const& it)
    {
        return (*it == VALUE);
    }


    template <int_t VALUE>
    class BitViewValues
    {
        using SkipIt_t = std::conditional_t<VALUE,
                SkipValueIterator< IT_T, ITB_T, stupid_test<0x0> >,
                SkipValueIterator< IT_T, ITB_T, stupid_test<~int_t(0x0)> > >;
        using ValueIt_t = BitValueIterator< SkipIt_t, ITB_T, VALUE >;
    public:

        constexpr BitViewValues(BitView const *pContainer)
         : m_pContainer{pContainer}
        { }

        constexpr ValueIt_t begin() const noexcept
        {
            auto skipBegin = SkipIt_t(m_pContainer->m_begin, m_pContainer->m_end);
            return {skipBegin, 0, skipBegin, m_pContainer->m_end};
        }
        constexpr ValueIt_t end() const noexcept
        {
            auto skipBegin = SkipIt_t(m_pContainer->m_begin, m_pContainer->m_end);
            auto skipEnd = SkipIt_t(m_pContainer->m_end, m_pContainer->m_end);
            return {skipEnd, 0, skipBegin, m_pContainer->m_end};
        }
    private:
        BitView const *m_pContainer;
    };

public:

    constexpr BitView(IT_T first, ITB_T last)
     : m_begin{first}
     , m_end{last}
    { }

    template <typename RANGE_T>
    constexpr BitView(RANGE_T& rRange)
     : BitView(std::begin(rRange), std::end(rRange))
    { }

    constexpr void set(std::size_t bit) noexcept;
    constexpr void set() noexcept;
    constexpr void reset(std::size_t bit) noexcept;
    constexpr void reset() noexcept;
    constexpr bool test(std::size_t bit) noexcept;

    constexpr std::size_t size() const noexcept;
    constexpr BitViewValues<true> const ones() const noexcept { return this; }
    constexpr BitViewValues<false> const zeros() const noexcept { return this; }

private:
    IT_T m_begin;
    ITB_T m_end;
};

} // namespace lgrn
