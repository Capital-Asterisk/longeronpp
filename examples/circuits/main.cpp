/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */

#include <longeron/containers/intarray_multimap.hpp>
#include <longeron/id_management/registry_stl.hpp>

#include <algorithm>
#include <iostream>
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
 * @brief Connects cirucit elements together
 */
struct Nodes
{
    // reminder: IntArrayMultiMap is kind of like an
    //           std::vector< std::vector<...> > but more memory efficient
    using Subscribers_t = lgrn::IntArrayMultiMap<uint32_t, ElementId>;
    using Connections_t = lgrn::IntArrayMultiMap<uint32_t, NodeId>;

    lgrn::IdRegistryStl<NodeId>             m_nodeIds;

    // Node-to-Element connections
    // Each node can have multiple subscribers, but only one publisher
    Subscribers_t                           m_nodeSubscribers;
    std::vector<ElementId>                  m_nodePublisher;

    // Corresponding Element-to-Node connections
    Connections_t                           m_elemConnect;
};

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
    // * PAIRITY - High when odd number of inputs High, chained XOR operations
    enum class Op : uint8_t { AND, OR, XOR, PAIRITY };

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

        std::cout << "Updating Element: " << elem << "\n";

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
        case Op::PAIRITY:
            value = (std::count_if(inFirst, inLast, is_logic_high) & 1) == 1;
            break;
        }

        std::for_each(inFirst, inLast, [pNodeValues] (NodeId in)
        {
            std::cout << "* sees: " << (pNodeValues[in] == ELogic::High) << "\n";
        });

        value ^= desc.m_invert;

        ELogic const outLogic = value ? ELogic::High : ELogic::Low;

        NodeId out = *connectedNodes.begin();

        // Request to write changes to node if value is changed
        if (pNodeValues[out] != outLogic)
        {
            std::cout << "-> Notify Node: " << out << "\n";
            nodeUpdated = true;
            rUpdNodes.m_nodeDirty.set(out);
            rUpdNodes.m_nodeNewValues[out] = outLogic;
        }
    }

    return nodeUpdated;
}

/**
 * @brief update_signals
 *
 * @param toUpdate
 * @param pNewValues
 * @param nodeSubs
 * @param pValues
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

        std::cout << "Updating node: " << char('A' + node) << "\n";

        // Notify subscribed elements
        for (ElementId subElem : nodeSubs[node])
        {
            std::cout << "-> notify Element: " << subElem << "\n";
            elemNotified = true;
            ElemTypeId const type = elements.m_elemTypes[subElem];
            uint32_t const dense = elements.m_elemSparse[subElem];
            rUpdElem[type].m_denseDirty.set(dense);
        }
    }

    return elemNotified;
}

} // namespace circuits




using namespace circuits;

constexpr auto const gc_elemGate = ElemTypeId(0);

struct NodePair
{
    struct ToElement
    {
        ElementId m_elem;
        int m_port;
    };

    ToElement m_publisher;
    std::initializer_list<ToElement> m_subscribers;
};

using lgrn::id_null;

int main(int argc, char** argv)
{
    // Forget inheritance ever existed
    Elements            elements;
    Nodes               logicNodes;
    NodeValues<ELogic>  logicValues;
    CombinationalGates  gates;

    int const maxElem = 64;
    int const maxNodes = 64;
    int const maxTypes = 2;

    elements.m_ids                  .reserve(maxElem);
    elements.m_elemSparse           .resize(maxElem, ~uint32_t(0));
    elements.m_elemTypes            .resize(maxElem, id_null<ElemTypeId>());
    logicNodes.m_nodeIds            .reserve(maxNodes);
    logicNodes.m_nodePublisher      .resize(maxNodes, id_null<ElementId>());
    logicNodes.m_nodeSubscribers    .ids_reserve(maxNodes);
    logicNodes.m_nodeSubscribers    .data_reserve(maxElem);
    logicNodes.m_elemConnect        .ids_reserve(maxElem);
    logicNodes.m_elemConnect        .data_reserve(maxNodes);
    logicValues.m_nodeValues        .resize(maxNodes);
    elements.m_typeDenseToElem      .resize(maxTypes);
    gates.m_elemGates               .reserve(maxElem);

    // A >-+------- =NAND1-+ D
    //     |       |       |
    //      =NAND0-+ C      =NAND3--> F (Out)
    //     |       |       |
    // B >-+------- =NAND2-+ E

    // Create 4 logic gates
    std::array<ElementId, 4> nands;
    elements.m_ids.create(nands.begin(), nands.end());

    using Op = CombinationalGates::Op;

    // Assign logic gates as NANDs
    for (ElementId id : nands)
    {
        auto &rDenseVec = elements.m_typeDenseToElem[gc_elemGate];
        uint32_t const denseIndex = rDenseVec.size();
        rDenseVec.emplace_back(id);

        elements.m_elemTypes[id] = gc_elemGate;
        elements.m_elemSparse[id] = denseIndex;
        gates.m_elemGates[id] = { Op::AND, true };
    }

    ElementId const null{id_null<ElementId>()};

    // connect logic gates
    std::array<NodePair, 6> const connections
    {{
        {  {null,     0},   { {nands[0], 1}, {nands[1], 1} }  }, // A
        {  {null,     0},   { {nands[0], 2}, {nands[2], 1} }  }, // B
        {  {nands[0], 0},   { {nands[1], 2}, {nands[2], 2} }  }, // C
        {  {nands[1], 0},   { {nands[3], 1} }                 }, // D
        {  {nands[2], 0},   { {nands[3], 2} }                 }, // E
        {  {nands[3], 0},   { }                               }  // F
    }};

    // Create nodes
    std::array<NodeId, 6> nodeIds;
    logicNodes.m_nodeIds.create(nodeIds.begin(), nodeIds.end());

    // create partitions for subscribers
    for (unsigned int i = 0; i < connections.size(); ++i)
    {
        logicNodes.m_nodeSubscribers.emplace(nodeIds[i], connections[i].m_subscribers.size());
    }

    // count number of connections for each element
    std::vector<int> elemConnectCount(maxElem, 0);

    for (NodePair const& pair : connections)
    {
        if (ElementId id = pair.m_publisher.m_elem; id != null) { elemConnectCount[id] ++; }
        for (NodePair::ToElement const& sub : pair.m_subscribers)
        {
            elemConnectCount[sub.m_elem] ++;
        }
    }

    // create partitions for element->node connections
    for (ElementId id : nands)
    {
        logicNodes.m_elemConnect.emplace(id, elemConnectCount[id]);
    }

    // make connections
    // populate logic.m_publisher, logic.m_nodeSubscribers, logic.m_elemConnect
    for (unsigned int i = 0; i < connections.size(); i ++)
    {
        NodePair const& pair = connections[i];
        NodeId const node = nodeIds[i];

        ElementId const publisher = pair.m_publisher.m_elem;
        logicNodes.m_nodePublisher[node] = publisher;
        if (pair.m_publisher.m_elem != null)
        {
            logicNodes.m_elemConnect[publisher][pair.m_publisher.m_port] = node;
        }

        auto subDestIt = logicNodes.m_nodeSubscribers[node].begin();
        auto subSrcIt = pair.m_subscribers.begin();
        for (unsigned int j = 0; j < pair.m_subscribers.size(); ++j)
        {
            logicNodes.m_elemConnect[subSrcIt->m_elem][subSrcIt->m_port] = node;

            *subDestIt = subSrcIt->m_elem;
            ++subDestIt;
            ++subSrcIt;
        }
    }

    std::cout << elements.m_ids.capacity() << "\n";


    UpdateElemTypes_t updElems;
    UpdateNodes<ELogic> updLogic;

    updElems.resize(maxTypes);

    updElems[gc_elemGate].m_denseDirty.ints().resize(2);
    updElems[gc_elemGate].m_denseDirty.set(0);
    updLogic.m_nodeDirty.ints().resize(2);
    updLogic.m_nodeNewValues.resize(maxNodes);

    auto const step_until_stable = [&elements, &logicNodes, &logicValues, &gates, &updLogic, &updElems] ()
    {
        bool elemNotified = true;
        while (elemNotified)
        {
            update_nodes(updLogic.m_nodeDirty.ones(), logicNodes.m_nodeSubscribers, elements, updLogic.m_nodeNewValues.data(), logicValues.m_nodeValues.data(), updElems);
            updLogic.m_nodeDirty.reset();

            elemNotified = update_combinational(updElems[gc_elemGate].m_denseDirty.ones(), elements.m_typeDenseToElem[gc_elemGate].data(), logicNodes.m_elemConnect, logicValues.m_nodeValues.data(), gates, updLogic);
            updElems[gc_elemGate].m_denseDirty.reset();
        }
    };

    // initialize to get to valid state
    for (uint32_t elem : elements.m_ids.bitview().zeros())
    {
        ElemTypeId const type = elements.m_elemTypes[elem];
        uint32_t const dense = elements.m_elemSparse[elem];
        updElems[type].m_denseDirty.set(dense);
    }

    updLogic.m_nodeNewValues[0] = ELogic::Low;
    updLogic.m_nodeNewValues[1] = ELogic::Low;
    updLogic.m_nodeDirty.set(0);
    updLogic.m_nodeDirty.set(1);
    step_until_stable();

    std::cout << "### 0 XOR 0 = " << (logicValues.m_nodeValues[nodeIds[5]] == ELogic::High) << "\n";

    updLogic.m_nodeNewValues[0] = ELogic::Low;
    updLogic.m_nodeNewValues[1] = ELogic::High;
    updLogic.m_nodeDirty.set(0);
    updLogic.m_nodeDirty.set(1);
    step_until_stable();

    std::cout << "### 0 XOR 1 = " << (logicValues.m_nodeValues[nodeIds[5]] == ELogic::High) << "\n";

    updLogic.m_nodeNewValues[0] = ELogic::High;
    updLogic.m_nodeNewValues[1] = ELogic::Low;
    updLogic.m_nodeDirty.set(0);
    updLogic.m_nodeDirty.set(1);
    step_until_stable();

    std::cout << "### 1 XOR 0 = " << (logicValues.m_nodeValues[nodeIds[5]] == ELogic::High) << "\n";

    updLogic.m_nodeNewValues[0] = ELogic::High;
    updLogic.m_nodeNewValues[1] = ELogic::High;
    updLogic.m_nodeDirty.set(0);
    updLogic.m_nodeDirty.set(1);
    step_until_stable();

    std::cout << "### 1 XOR 1 = " << (logicValues.m_nodeValues[nodeIds[5]] == ELogic::High) << "\n";

    return 0;
}

