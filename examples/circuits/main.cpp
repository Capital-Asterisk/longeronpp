/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */

#include "circuits.hpp"

#include <longeron/containers/intarray_multimap.hpp>
#include <longeron/id_management/registry_stl.hpp>

#include <algorithm>
#include <iostream>
#include <vector>


using namespace circuits;

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

