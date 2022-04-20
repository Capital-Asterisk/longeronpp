/**
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2022 Neal Nicdao <chrisnicdao0@gmail.com>
 */
#pragma once

#include "circuits.hpp"

#include <array>
#include <algorithm>

namespace circuits
{

template<typename VALUE_T>
struct WipNodes
{
    static inline thread_local Nodes *smt_pNodes{nullptr};
};

inline thread_local Elements *t_wipElements{nullptr};
inline thread_local CombinationalGates *t_wipGates{nullptr};

template <std::size_t N, typename VALUE_T>
std::array<NodeId, N> create_nodes() noexcept
{
    Nodes *pNodes = WipNodes<VALUE_T>::smt_pNodes;
    LGRN_ASSERTM(pNodes != nullptr, "No logic nodes in construction");

    std::array<NodeId, N> out;
    pNodes->m_nodeIds.create(std::begin(out), std::end(out));
    return out;
}

//-----------------------------------------------------------------------------

// Logic and combinational

ElementId gate_combinatinal(CombinationalGates::GateDesc desc, std::initializer_list<NodeId> in, NodeId out);

inline ElementId gate_AND(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::AND, false }, in, out); }

inline ElementId gate_NAND(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::AND, true }, in, out); }

inline ElementId gate_OR(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::OR, false }, in, out); }

inline ElementId gate_NOR(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::OR, true }, in, out); }

inline ElementId gate_XOR(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::XOR, false }, in, out); }

inline ElementId gate_XNOR(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::XOR, true }, in, out); }

inline ElementId gate_XOR2(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::XOR2, false }, in, out); }

inline ElementId gate_XNOR2(std::initializer_list<NodeId> in, NodeId out)
{ return gate_combinatinal({ CombinationalGates::Op::XOR2, true }, in, out); }

void populate_pub_sub(Elements const& elements, Nodes &rNodes);



} // namespace circuits
