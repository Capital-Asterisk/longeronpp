/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2024 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "../utility/enum_traits.hpp"

#include <vector>

namespace lgrn
{

/**
 * @brief Wraps an std::vector intended to be accessed using a (strong typedef) enum class ID
 */
template <typename ID_T, typename DATA_T, typename ALLOC_T = std::allocator<DATA_T>>
class KeyedVec : public std::vector<DATA_T, ALLOC_T>
{
    using vector_t  = std::vector<DATA_T, ALLOC_T>;
    using int_t     = lgrn::underlying_int_type_t<ID_T>;

public:

    using value_type                = typename vector_t::value_type;
    using allocator_type            = typename vector_t::allocator_type;
    using reference                 = typename vector_t::reference;
    using const_reference           = typename vector_t::const_reference;
    using pointer                   = typename vector_t::pointer;
    using const_pointer             = typename vector_t::const_pointer;
    using iterator                  = typename vector_t::iterator;
    using const_iterator            = typename vector_t::const_iterator;
    using reverse_iterator          = typename vector_t::reverse_iterator;
    using const_reverse_iterator    = typename vector_t::const_reverse_iterator;
    using difference_type           = typename vector_t::difference_type;
    using size_type                 = typename vector_t::size_type;

    constexpr vector_t& base() noexcept { return *this; }
    constexpr vector_t const& base() const noexcept { return *this; }

    reference at(ID_T const id)
    {
        return vector_t::at(std::size_t(id));
    }

    const_reference at(ID_T const id) const
    {
        return vector_t::at(std::size_t(id));
    }

    reference operator[] (ID_T const id)
    {
        return vector_t::operator[](std::size_t(id));
    }

    const_reference operator[] (ID_T const id) const
    {
        return vector_t::operator[](std::size_t(id));
    }

}; // class KeyedVec



} // namespace lgrn
