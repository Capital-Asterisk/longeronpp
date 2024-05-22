/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include <iterator>

namespace lgrn
{

template <typename BASE_T, typename ID_T>
class IdCastIterator : public BASE_T
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = ID_T;
    using pointer           = void;
    using reference         = void;

    constexpr value_type operator*() const noexcept
    {
        return ID_T(BASE_T::operator*());
    }
};

} // namespace lgrn
