/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "../utility/enum_traits.hpp"

#include <limits>

namespace lgrn
{

template<class TYPE_T>
constexpr TYPE_T id_null() noexcept
{
    using id_int_t = underlying_int_type_t<TYPE_T>;
    return TYPE_T(std::numeric_limits<id_int_t>::max());
}

} // namespace lgrn
