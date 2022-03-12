/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "null.hpp"

#include <cassert>
#include <utility>

namespace lgrn
{

/**
 * @brief Long term storage for IDs
 */
template<typename ID_T, typename REG_T>
class IdStorage
{
    friend REG_T;

public:
    IdStorage() : m_id{ id_null<ID_T>() } { }
    IdStorage(IdStorage&& move)
     : m_id{std::exchange(move.m_id, id_null<ID_T>())}
    { }
    ~IdStorage() { assert( ! has_value() ); }

    // Delete copy
    IdStorage(IdStorage const& copy) = delete;
    IdStorage& operator=(IdStorage const& copy) = delete;

    // Allow move assignment only if there's no ID stored
    IdStorage& operator=(IdStorage&& move)
    {
        assert( ! has_value() );
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
