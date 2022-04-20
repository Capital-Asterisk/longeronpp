/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */

#include "circuits.hpp"
#include "circuit_builder.hpp"

#include <longeron/containers/intarray_multimap.hpp>
#include <longeron/id_management/registry_stl.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

#include <optional>


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

void create_xor_nand(NodeId A, NodeId B, NodeId out)
{
    // A >-+------- =NAND1-+ D
    //     |       |       |
    //      =NAND0-+ C      =NAND3--> Out
    //     |       |       |
    // B >-+------- =NAND2-+ E

    auto const [C, D, E] = create_nodes<3, ELogic>();

    gate_NAND({A, B}, C);
    gate_NAND({A, C}, D);
    gate_NAND({C, B}, E);
    gate_NAND({D, E}, out);
}

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


    t_wipElements = &elements;
    t_wipGates = &gates;
    WipNodes<ELogic>::smt_pNodes = &logicNodes;


    auto const [A, B, out] = create_nodes<3, ELogic>();

    create_xor_nand(A, B, out);

    // Populate m_nodePublishers and m_nodeSubscribers
    populate_pub_sub(elements, logicNodes);

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

    updLogic.assign(A, ELogic::Low);
    updLogic.assign(B, ELogic::Low);
    step_until_stable();

    std::cout << "### 0 XOR 0 = " << (logicValues.m_nodeValues[out] == ELogic::High) << "\n";

    updLogic.assign(A, ELogic::Low);
    updLogic.assign(B, ELogic::High);
    step_until_stable();

    std::cout << "### 0 XOR 1 = " << (logicValues.m_nodeValues[out] == ELogic::High) << "\n";

    updLogic.assign(A, ELogic::High);
    updLogic.assign(B, ELogic::Low);
    step_until_stable();

    std::cout << "### 1 XOR 0 = " << (logicValues.m_nodeValues[out] == ELogic::High) << "\n";

    updLogic.assign(A, ELogic::High);
    updLogic.assign(B, ELogic::High);
    step_until_stable();

    std::cout << "### 1 XOR 1 = " << (logicValues.m_nodeValues[out] == ELogic::High) << "\n";

    return 0;
}

