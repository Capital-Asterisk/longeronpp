/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */

#include "circuit_builder.hpp"

namespace circuits
{

ElementId gate_combinatinal(CombinationalGates::GateDesc desc, std::initializer_list<NodeId> in, NodeId out)
{
    Nodes *pNodes = WipNodes<ELogic>::smt_pNodes;
    LGRN_ASSERTM(pNodes != nullptr, "No logic nodes in construction");
    LGRN_ASSERTM(t_wipElements != nullptr, "No elements in construction");
    LGRN_ASSERTM(t_wipGates != nullptr, "No elements in construction");

    PerElemType &rPerType = t_wipElements->m_perType[gc_elemGate];

    // Create Element Id and Local Id
    ElementId const elemId = t_wipElements->m_ids.create();
    ElemLocalId const localId = rPerType.m_localIds.create();

    // Assign Type and Local ID
    rPerType.m_localToElem[localId] = elemId;
    t_wipElements->m_elemTypes[elemId] = gc_elemGate;
    t_wipElements->m_elemToLocal[elemId] = localId;

    // Assign gate description
    t_wipGates->m_localGates[localId] = desc;

    NodeId *pData = pNodes->m_elemConnect.emplace(elemId, in.size() + 1);

    pData[0] = out; // Port 0 is output
    std::copy(std::begin(in), std::end(in), pData + 1); // the rest are inputs

    return elemId;
}

void populate_pub_sub(Elements const& elements, Nodes &rNodes)
{
    std::vector<int> nodeSubCount(rNodes.m_nodeIds.capacity(), 0); // can we reach 1 million subscribers?
    for (ElementId elem : elements.m_ids)
    {
        auto nodes = rNodes.m_elemConnect[elem];
        // skip first, port 0 is the publisher
        for (auto it = nodes.begin() + 1; it != nodes.end(); ++it)
        {
            nodeSubCount[*it] ++;
        }
    }
    // reserve subscriber partitions
    for (NodeId node : rNodes.m_nodeIds.bitview().zeros())
    {
        rNodes.m_nodeSubscribers.emplace(node, nodeSubCount[node]);
    }
    // assign publishers and subscribers
    for (ElementId elem : elements.m_ids.bitview().zeros())
    {
        auto nodes = rNodes.m_elemConnect[elem];
        for (auto it = nodes.begin() + 1; it != nodes.end(); ++it)
        {
            // A bit hacky, but now using subscriber counts to keep track of how
            // many subscribers had currently been added
            int &rSubCount = nodeSubCount[*it];
            rSubCount --;

            ElemTypeId const type = elements.m_elemTypes[elem];
            ElemLocalId const local = elements.m_elemToLocal[elem];

            rNodes.m_nodeSubscribers[*it][rSubCount] = {local, type};
        }

        // assign publisher
        rNodes.m_nodePublisher[nodes[0]] = elem;
    }
}

}
