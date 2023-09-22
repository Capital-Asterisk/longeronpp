/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2023 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

namespace lgrn
{

template <typename IT_T, typename ITB_T>
struct IteratorPair
{
    constexpr IteratorPair(IT_T first, ITB_T last)
     : m_begin{first}
     , m_end{last}
    { }

    constexpr IT_T begin() const noexcept { return m_begin; }
    constexpr IT_T end() const noexcept { return m_end; }

    IT_T m_begin;
    ITB_T m_end;
};

} // namespace lgrn
