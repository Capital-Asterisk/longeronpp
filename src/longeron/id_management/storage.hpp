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
class IdStorage
{
    friend REG_T;

public:
    IdStorage() : m_id{ id_null<ID_T>() } { }
    IdStorage(IdStorage&& move) = default;
    ~IdStorage()
    {
        LGRN_ASSERTMV( ! has_value(), "IdStorage's value must be cleared by its friend class before destruction",
                      std::size_t(m_id));
    }

    // Delete copy
    IdStorage(IdStorage const& copy) = delete;
    IdStorage& operator=(IdStorage const& copy) = delete;

    // Allow move assignment only if there's no ID stored
    IdStorage& operator=(IdStorage&& move)
    {
        LGRN_ASSERTMV( ! has_value(), "IdStorage's value must be cleared by its friend class before replacing stored value",
                      std::size_t(m_id), std::size_t(move.m_id));
        m_id = std::exchange(move.m_id, id_null<ID_T>());
        return *this;
    }

    operator ID_T() const noexcept { return m_id; }
    constexpr ID_T value() const noexcept { return m_id; }
    constexpr bool has_value() const noexcept { return m_id != id_null<ID_T>(); }

private:
    IdStorage(ID_T id) : m_id{ id } { }

    ID_T m_id;

}; // class IdStorage

} // namespace lgrn
