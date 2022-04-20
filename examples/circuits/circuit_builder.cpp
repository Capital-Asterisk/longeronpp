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

    ElementId const id = t_wipElements->m_ids.create();

    auto &rDenseVec = t_wipElements->m_typeDenseToElem[gc_elemGate];
    uint32_t const denseIndex = rDenseVec.size();
    rDenseVec.emplace_back(id);

    t_wipElements->m_elemTypes[id] = gc_elemGate;
    t_wipElements->m_elemSparse[id] = denseIndex;
    t_wipGates->m_elemGates[id] = desc;

    NodeId *pData = pNodes->m_elemConnect.emplace(id, in.size() + 1);

    pData[0] = out; // Port 0 is output
    std::copy(std::begin(in), std::end(in), pData + 1); // the rest are inputs

    return id;
}

void populate_pub_sub(Elements const& elements, Nodes &rNodes)
{
    std::vector<int> nodeSubCount(rNodes.m_nodeIds.capacity(), 0); // can we reach 1 million subscribers?
    for (ElementId elem : elements.m_ids.bitview().zeros())
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

            rNodes.m_nodeSubscribers[*it][rSubCount] = elem;
        }

        // assign publisher
        rNodes.m_nodePublisher[nodes[0]] = elem;
    }
}

}
