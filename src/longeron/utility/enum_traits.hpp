/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 * SPDX-FileContributor: Michael Jones <jonesmz@jonesmz.com>
 */
#pragma once

#include <type_traits>

namespace lgrn
{

template<typename TYPE_T>
struct type_identity
{
    using type = TYPE_T;
};

template<typename TYPE_T>
using type_identity_t = typename type_identity<TYPE_T>::type;

/**
 * @brief Obtain underlying type if given an enum, or same type if given any
 *        integer type
 *
 * std::underlying_type will cause a compile error if given a normal integer.
 *
 * This template struct can be used in cases where a template parameter is
 * allowed to be an enum or an int.
 */
template<typename TYPE_T, typename = void>
struct underlying_int_type;

template<typename TYPE_T>
struct underlying_int_type< TYPE_T, std::enable_if_t< std::is_enum_v<TYPE_T> > >
 : std::underlying_type<TYPE_T>
{ };

template<typename TYPE_T>
struct underlying_int_type< TYPE_T, std::enable_if_t< ! std::is_enum_v<TYPE_T> > >
 : type_identity<TYPE_T>
{ };

template<typename TYPE_T>
using underlying_int_type_t = typename underlying_int_type<TYPE_T>::type;

} // namespace lgrn
