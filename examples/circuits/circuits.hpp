/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include <longeron/containers/intarray_multimap.hpp>
#include <longeron/id_management/id_set_stl.hpp>
#include <longeron/id_management/keyed_vec_stl.hpp>
#include <longeron/id_management/registry_stl.hpp>

#include <algorithm>
#include <vector>

namespace circuits
{


// Enum classes used as integer "strong typedef" and can't be implicitly casted to each other,
// preventing logic errors and cognative load from accidentally mixing them up.
enum class ElemLocalId : std::uint32_t { };
enum class ElemTypeId  : std::uint8_t  { };

// lgrn::IntArrayMultiMap does not (yet?) support strongly typedefed ID types
using ElementId = std::uint32_t;
using NodeId    = std::uint32_t;


/**
 * @brief Keeps track of which circuit elements of a certain type exists
 */
struct PerElemType
{
    lgrn::IdRegistryStl<ElemLocalId>       m_localIds;
    lgrn::KeyedVec<ElemLocalId, ElementId> m_localToElem;
};

/**
 * @brief Keeps track of which circuit elements exist and what type they are
 */
struct Elements
{
    lgrn::IdRegistryStl<ElementId>          m_ids;

    lgrn::KeyedVec<ElementId, ElemTypeId>   m_elemTypes;
    lgrn::KeyedVec<ElementId, ElemLocalId>  m_elemToLocal;

    lgrn::KeyedVec<ElemTypeId, PerElemType> m_perType;
};

/**
 * @brief Refers to an Element using Type and Local Id instead of an Element Id
 */
struct ElementPair
{
    ElemLocalId     m_id;
    ElemTypeId      m_type;
};


/**
 * @brief Connects cirucit elements together as Nodes
 */
struct Nodes
{
    // reminder: IntArrayMultiMap is kind of like an
    //           std::vector< std::vector<...> > but more memory efficient
    using Subscribers_t = lgrn::IntArrayMultiMap<NodeId, ElementPair>;
    using Connections_t = lgrn::IntArrayMultiMap<ElementId, NodeId>;

    lgrn::IdRegistryStl<NodeId>             m_nodeIds;

    // Node-to-Element connections
    // Each node can have multiple subscribers, but only one publisher
    Subscribers_t                           m_nodeSubscribers;
    lgrn::KeyedVec<NodeId, ElementId>       m_nodePublisher;

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
    lgrn::KeyedVec<NodeId, VALUE_T> m_nodeValues;
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

    lgrn::KeyedVec<ElemLocalId, GateDesc> m_localGates;
};

//-----------------------------------------------------------------------------

// Updating

constexpr std::size_t const gc_bitVecIntSize = 64;

struct UpdateElem
{
    lgrn::IdSetStl<ElemLocalId> m_localDirty;
};

using UpdateElemTypes_t = lgrn::KeyedVec<ElemTypeId, UpdateElem>;

template <typename VALUE_T>
struct UpdateNodes
{
    lgrn::IdSetStl<NodeId>          m_nodeDirty;
    lgrn::KeyedVec<NodeId, VALUE_T> m_nodeNewValues;

    void assign(NodeId node, VALUE_T&& value)
    {
        m_nodeDirty.insert(node);
        m_nodeNewValues[node] = std::forward<VALUE_T>(value);
    }
};

/**
 * @brief Update Combinational Logic Gates and request node changes
 *
 * @param[in] toUpdate      Iterable range of element dence indices to update
 * @param[in] pLocalToElem  Array to get Element IDs from Local IDs
 * @param[in] elemConnect   Element->Node connections
 * @param[in] pNodeValues   Values of logic nodes, used to detect changes
 * @param[in] gates         Data for combinational logic gates
 * @param[out] rUpdNodes    Node changes out
 *
 * @return true if any node changes are written
 */
template <typename RANGE_T>
bool update_combinational(
        RANGE_T&&                           toUpdate,
        lgrn::KeyedVec<ElemLocalId, ElementId> const &localToElem,
        Nodes::Connections_t          const &elemConnect,
        lgrn::KeyedVec<NodeId, ELogic>      &nodeValues,
        CombinationalGates            const &gates,
        UpdateNodes<ELogic>                 &rUpdNodes) noexcept
{
    using Op = CombinationalGates::Op;

    auto const is_logic_high = [&nodeValues] (NodeId in) noexcept -> bool
    {
        return nodeValues[in] == ELogic::High;
    };

    bool nodeUpdated = false;

    for (ElemLocalId local : toUpdate)
    {
        ElementId                    const elem  = localToElem[local];
        CombinationalGates::GateDesc const &desc = gates.m_localGates[local];

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
        if (nodeValues[out] != outLogic)
        {
            nodeUpdated = true;
            rUpdNodes.m_nodeDirty.insert(out);
            rUpdNodes.m_nodeNewValues[out] = outLogic;
        }
    }

    return nodeUpdated;
}

/**
 * @brief Update node values and notify subscribed Elements
 *
 * @param[in] toUpdate   Range of nodes to update
 * @param[in] nodeSubs   Subscribers of each node
 * @param[in] elements   Element data
 * @param[in] pNewValues New node values
 * @param[out] pValues   Node values to write to
 * @param[out] rUpdElem  Element notifications output
 *
 * @return true if any nodes were notified
 */
template <typename VALUE_T, typename RANGE_T>
bool update_nodes(
        RANGE_T                                 &&toUpdate,
        Nodes::Subscribers_t              const &nodeSubs,
        Elements                          const &elements,
        lgrn::KeyedVec<NodeId, VALUE_T>   const &newValues,
        lgrn::KeyedVec<NodeId, VALUE_T>         &values,
        UpdateElemTypes_t                       &rUpdElem)
{
    bool elemNotified = false;

    for (NodeId node : toUpdate)
    {
        // Apply node value changes
        values[node] = newValues[node];

        // Notify subscribed elements
        for (ElementPair subElem : nodeSubs[node])
        {
            elemNotified = true;
            rUpdElem[subElem.m_type].m_localDirty.insert(subElem.m_id);
        }
    }

    return elemNotified;
}

constexpr auto const gc_elemGate = ElemTypeId(0);

} // namespace circuits
