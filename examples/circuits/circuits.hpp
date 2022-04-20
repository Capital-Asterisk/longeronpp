/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include <longeron/containers/intarray_multimap.hpp>
#include <longeron/id_management/registry_stl.hpp>

#include <algorithm>
#include <vector>

namespace circuits
{

using ElementId     = uint32_t;
using ElemTypeId    = uint8_t;
using NodeId        = uint32_t;

/**
 * @brief Keeps track of which circuit elements exist and what type they are
 */
struct Elements
{
    lgrn::IdRegistryStl<ElementId>          m_ids;

    std::vector<uint32_t>                   m_elemSparse;

    // access with [ElemTypeId][sparse index]
    std::vector<ElemTypeId>                 m_elemTypes;
    std::vector< std::vector<ElementId> >   m_typeDenseToElem;
};

/**
 * @brief Connects cirucit elements together as Nodes
 */
struct Nodes
{
    // reminder: IntArrayMultiMap is kind of like an
    //           std::vector< std::vector<...> > but more memory efficient
    using Subscribers_t = lgrn::IntArrayMultiMap<NodeId, ElementId>;
    using Connections_t = lgrn::IntArrayMultiMap<ElementId, NodeId>;

    lgrn::IdRegistryStl<NodeId>             m_nodeIds;

    // Node-to-Element connections
    // Each node can have multiple subscribers, but only one publisher
    Subscribers_t                           m_nodeSubscribers;
    std::vector<ElementId>                  m_nodePublisher;

    // Corresponding Element-to-Node connections
    // [ElementId][PortId] -> NodeId
    Connections_t                           m_elemConnect;
};

/**
 * @brief Associates values with Nodes
 *
 * Note: Logic gates themselves are stateless! Intermediate values (aka Nodes)
 *       are what matters the most.
 */
template <typename VALUE_T>
struct NodeValues
{
    std::vector<VALUE_T> m_nodeValues;
};

//-----------------------------------------------------------------------------

// Logic value support

enum class ELogic : uint8_t { Low = 0, High = 1 };

struct CombinationalGates
{
    // Behaviour of a 'multi-input XOR gate' is disputed, either:
    // * XOR - High when only a single input is High, correct to 'exclusive' OR
    // * XOR2 - High when odd number of inputs High, chained XORs / Parity
    enum class Op : uint8_t { AND, OR, XOR, XOR2 };

    struct GateDesc
    {
        Op m_op;
        bool m_invert;
    };

    std::vector<GateDesc> m_elemGates;
};

//-----------------------------------------------------------------------------

// Updating

struct UpdateElem
{
    lgrn::BitView< std::vector<uint64_t> >  m_denseDirty;
};

using UpdateElemTypes_t = std::vector<UpdateElem>;

template <typename VALUE_T>
struct UpdateNodes
{
    lgrn::BitView< std::vector<uint64_t> >  m_nodeDirty;
    std::vector<VALUE_T>                    m_nodeNewValues;

    void assign(NodeId node, VALUE_T&& value)
    {
        m_nodeDirty.set(node);
        m_nodeNewValues[node] = std::forward<VALUE_T>(value);
    }
};

//-----------------------------------------------------------------------------

/**
 * @brief
 *
 * @param[in] toUpdate      Iterable range of element dence indices to update
 * @param[in] pDenseElem    ElementIDs from dence indices, indexed by toUpdate
 * @param[in] elemConnect   Element->Node connections
 * @param[in] pNodeValues   Values of logic nodes, used to detect changes
 * @param[in] gates         Data for combinational logic gates
 * @param[out] rUpdNodes    Node changes out
 */
template <typename RANGE_T>
bool update_combinational(
        RANGE_T&&                       toUpdate,
        ElementId const*                pDenseElem,
        Nodes::Connections_t const&     elemConnect,
        ELogic const*                   pNodeValues,
        CombinationalGates const&       gates,
        UpdateNodes<ELogic>&            rUpdNodes) noexcept
{
    using Op = CombinationalGates::Op;

    auto const is_logic_high = [pNodeValues] (NodeId in) noexcept -> bool
    {
        return pNodeValues[in] == ELogic::High;
    };

    bool nodeUpdated = false;

    for (uint32_t dense : toUpdate)
    {
        ElementId const elem = pDenseElem[dense];
        CombinationalGates::GateDesc const& desc = gates.m_elemGates[dense];

        auto connectedNodes = elemConnect[elem];
        auto inFirst = connectedNodes.begin() + 1;
        auto inLast  = connectedNodes.end();

        // Read input nodes and compute operation
        bool value;
        switch (desc.m_op)
        {
        case Op::AND:
            value = std::all_of(inFirst, inLast, is_logic_high);
            break;
        case Op::OR:
            value = std::any_of(inFirst, inLast, is_logic_high);
            break;
        case Op::XOR:
            value = std::count_if(inFirst, inLast, is_logic_high) == 1;
            break;
        case Op::XOR2:
            value = (std::count_if(inFirst, inLast, is_logic_high) & 1) == 1;
            break;
        }

        value ^= desc.m_invert;

        ELogic const outLogic = value ? ELogic::High : ELogic::Low;

        NodeId out = *connectedNodes.begin();

        // Request to write changes to node if value is changed
        if (pNodeValues[out] != outLogic)
        {
            nodeUpdated = true;
            rUpdNodes.m_nodeDirty.set(out);
            rUpdNodes.m_nodeNewValues[out] = outLogic;
        }
    }

    return nodeUpdated;
}

/**
 * @brief update_nodes
 *
 * @param toUpdate
 * @param nodeSubs
 * @param elements
 * @param pNewValues
 * @param pValues
 * @param rUpdElem
 * @return
 */
template <typename VALUE_T, typename RANGE_T>
bool update_nodes(
        RANGE_T&&                       toUpdate,
        Nodes::Subscribers_t const&     nodeSubs,
        Elements const&                 elements,
        VALUE_T const*                  pNewValues,
        VALUE_T*                        pValues,
        UpdateElemTypes_t&              rUpdElem)
{
    bool elemNotified = false;

    for (uint32_t node : toUpdate)
    {
        // Apply node value changes
        pValues[node] = pNewValues[node];

        // Notify subscribed elements
        for (ElementId subElem : nodeSubs[node])
        {
            elemNotified = true;
            ElemTypeId const type = elements.m_elemTypes[subElem];
            uint32_t const dense = elements.m_elemSparse[subElem];
            rUpdElem[type].m_denseDirty.set(dense);
        }
    }

    return elemNotified;
}

constexpr auto const gc_elemGate = ElemTypeId(0);

} // namespace circuits
