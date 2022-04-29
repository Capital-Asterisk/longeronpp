/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "null.hpp"

#include "../utility/asserts.hpp"
#include "../utility/enum_traits.hpp"

#include <utility>

namespace lgrn
{

/**
 * @brief Enforces a ownership for a wrapped int/enum ID by preventing copy and
 *        improper destruction.
 *
 * Intended for unique or reference-counted IDs. Internal state can only be
 * modified by a friend class specified by template.
 *
 * Asserts are used for unintended moves and assignments which are only enforced
 * in debug. This class should be identical to a raw ID type on release.
 */
template<typename ID_T, typename REG_T>
class IdOwner
{
    friend REG_T;

public:
    IdOwner() : m_id{ id_null<ID_T>() } { }
    IdOwner(IdOwner&& move)
     : m_id{std::exchange(move.m_id, id_null<ID_T>())}
    { }
    ~IdOwner()
    {
        LGRN_ASSERTMV( ! has_value(), "IdOwner's value must be cleared by its friend class before destruction",
                      std::size_t(m_id));
    }

    // Delete copy
    IdOwner(IdOwner const& copy) = delete;
    IdOwner& operator=(IdOwner const& copy) = delete;

    // Allow move assignment only if there's no ID stored
    IdOwner& operator=(IdOwner&& move)
    {
        LGRN_ASSERTMV( ! has_value(), "IdOwner's value must be cleared by its friend class before replacing stored value",
                      std::size_t(m_id), std::size_t(move.m_id));
        m_id = std::exchange(move.m_id, id_null<ID_T>());
        return *this;
    }

    operator ID_T() const noexcept { return m_id; }
    constexpr ID_T value() const noexcept { return m_id; }
    constexpr bool has_value() const noexcept { return m_id != id_null<ID_T>(); }

private:
    IdOwner(ID_T id) : m_id{ id } { }

    ID_T m_id;

}; // class IdOwner

} // namespace lgrn
