/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "registry.hpp"
#include "storage.hpp"

namespace lgrn
{

/**
 * @brief An IdRegistry that uses IdStorages to uniquely store Ids
 */
template<typename ID_T, bool NO_AUTO_RESIZE = false>
class UniqueIdRegistry : IdRegistry<ID_T, NO_AUTO_RESIZE>
{
    using base_t = IdRegistry<ID_T, NO_AUTO_RESIZE>;
public:

    using Storage_t = IdStorage<ID_T, UniqueIdRegistry<ID_T, NO_AUTO_RESIZE> >;

    using IdRegistry<ID_T, NO_AUTO_RESIZE>::IdRegistry;
    using base_t::capacity;
    using base_t::size;
    using base_t::reserve;
    using base_t::exists;

    [[nodiscard]] Storage_t create()
    {
        return base_t::create();
    }

    void remove(Storage_t& rStorage)
    {
        base_t::remove(rStorage);
        rStorage.release();
    }

}; // class UniqueIdRegistry

} // namespace lgrn
