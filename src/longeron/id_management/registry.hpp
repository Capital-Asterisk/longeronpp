/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2021 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "null.hpp"
#include "../containers/hierarchical_bitset.hpp"
#include "../utility/enum_traits.hpp"
#include "../utility/asserts.hpp"

namespace lgrn
{

/**
 * @brief Generates reusable sequential IDs
 */
template<typename ID_T, bool NO_AUTO_RESIZE = false>
class IdRegistry
{
    using id_int_t = underlying_int_type_t<ID_T>;

public:

    IdRegistry() = default;
    IdRegistry(std::size_t capacity) { reserve(capacity); };

    /**
     * @brief Create a single ID
     *
     * @return A newly created ID
     */
    [[nodiscard]] ID_T create()
    {
        ID_T output{ id_null<ID_T>() };
        create(&output, 1);
        return output;
    }

    /**
     * @brief Create multiple IDs
     *
     * @param out   [out] Iterator out
     * @param count [in] Number of IDs to generate
     */
    template<typename IT_T>
    void create(IT_T out, std::size_t count);

    /**
     * @return Size required to fit all existing IDs, or allocated size if
     *         reserved ahead of time
     */
    constexpr std::size_t capacity() const noexcept { return m_deleted.size(); }

    /**
     * @return Number of registered IDs
     */
    constexpr std::size_t size() const { return capacity() - m_deleted.count(); }

    /**
     * @brief Allocate space for at least n IDs
     *
     * @param n [in] Number of IDs to allocate for
     */
    void reserve(std::size_t n)
    {
        m_deleted.resize(n, true);
    }

    /**
     * @brief Remove an ID. This will mark it for reuse
     *
     * @param id [in] ID to remove
     */
    void remove(ID_T id)
    {
        LGRN_ASSERTMV(exists(id), "ID does not exist", std::size_t(id));

        m_deleted.set(id_int_t(id));
    }

    /**
     * @brief Check if an ID exists
     *
     * @param id [in] ID to check
     *
     * @return true if ID exists
     */
    bool exists(ID_T id) const noexcept
    {
        return ! m_deleted.test(id_int_t(id));
    };

private:
    HierarchicalBitset<uint64_t> m_deleted;

}; // class IdRegistry

template<typename ID_T, bool NO_AUTO_RESIZE>
template<typename IT_T>
void IdRegistry<ID_T, NO_AUTO_RESIZE>::create(IT_T out, std::size_t count)
{
    if (m_deleted.count() < count)
    {
        // auto-resize
        LGRN_ASSERTMV(!NO_AUTO_RESIZE, "Reached max capacity with automatic resizing disabled", count, capacity());
        reserve(std::max(capacity() + count, capacity() * 2));
    }

    m_deleted.take<IT_T, ID_T>(out, count);
}

} // namespace lgrn
