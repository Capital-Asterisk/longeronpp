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

using lgrn::id_null;

/**
 * @brief Circuit types used in this example composed together into a struct
 *
 * Intended for convenience
 */
struct UserCircuit
{
    UserCircuit(std::size_t maxElem, std::size_t maxNodes, std::size_t maxTypes)
     : m_maxElem{maxElem}
     , m_maxNodes{maxNodes}
     , m_maxTypes{maxTypes}
    {
        // Viciously allocate enough space for everything
        m_elements.m_ids                .reserve(maxElem);
        m_elements.m_elemToLocal        .reserve(maxElem);
        m_elements.m_elemTypes          .resize(maxElem, id_null<ElemTypeId>());
        m_logicNodes.m_nodeIds          .reserve(maxNodes);
        m_logicNodes.m_nodePublisher    .resize(maxNodes, id_null<ElementId>());
        m_logicNodes.m_nodeSubscribers  .ids_reserve(maxNodes);
        m_logicNodes.m_nodeSubscribers  .data_reserve(maxElem);
        m_logicNodes.m_elemConnect      .ids_reserve(maxElem);
        m_logicNodes.m_elemConnect      .data_reserve(maxNodes);
        m_logicValues.m_nodeValues      .resize(maxNodes);
        m_gates.m_localGates            .reserve(maxElem);

        m_elements.m_perType            .resize(maxTypes);
        for (PerElemType &rPerType : m_elements.m_perType)
        {
            rPerType.m_localIds.reserve(maxElem);
            rPerType.m_localToElem.reserve(maxElem);
        }
    }

    void build_begin()
    {
        t_wipElements                   = &m_elements;
        t_wipGates                      = &m_gates;
        WipNodes<ELogic>::smt_pNodes    = &m_logicNodes;
    }

    void build_end()
    {
        t_wipElements                   = nullptr;
        t_wipGates                      = nullptr;
        WipNodes<ELogic>::smt_pNodes    = nullptr;

        populate_pub_sub(m_elements, m_logicNodes);
    }

    UpdateElemTypes_t setup_element_updater()
    {
        UpdateElemTypes_t out;
        out.resize(m_maxTypes);
        out[gc_elemGate].m_localDirty.ints().resize(m_elements.m_perType[gc_elemGate].m_localIds.vec().capacity());

        // Initially set all to dirty get to valid state
        // middle parts of a circuit may be unresponsive otherwise
        for (uint32_t elem : m_elements.m_ids.bitview().zeros())
        {
            ElemTypeId const type = m_elements.m_elemTypes[elem];
            ElemLocalId const local = m_elements.m_elemToLocal[elem];
            out[type].m_localDirty.set(local);
        }

        return out;
    }

    UpdateNodes<ELogic> setup_logic_updater()
    {
        UpdateNodes<ELogic> out;
        out.m_nodeDirty.ints().resize(m_maxNodes / gc_bitVecIntSize + 1);
        out.m_nodeNewValues.resize(m_maxNodes);

        return out;
    }

    // Forget inheritance ever existed
    Elements            m_elements;
    Nodes               m_logicNodes;
    NodeValues<ELogic>  m_logicValues;
    CombinationalGates  m_gates;

    std::size_t m_maxElem{0};
    std::size_t m_maxNodes{0};
    std::size_t m_maxTypes{0};
};

struct Waveform
{
    NodeId m_node;
    std::string_view m_wave;
};

constexpr bool is_high(ELogic in)
{
    return in == ELogic::High;
}

/**
 * @brief Step a circuit through time, stop when no more things change
 *
 * @return Actual number of steps taken
 */
static int step_until_stable(
        UserCircuit& rCircuit,
        UpdateNodes<ELogic>& rUpdLogic,
        UpdateElemTypes_t& rUpdElems,
        int maxSteps)
{
    int steps = 0;
    bool elemNotified = true;
    while (elemNotified && steps < maxSteps)
    {
        update_nodes(
                rUpdLogic.m_nodeDirty.ones(),
                rCircuit.m_logicNodes.m_nodeSubscribers,
                rCircuit.m_elements,
                rUpdLogic.m_nodeNewValues.data(),
                rCircuit.m_logicValues.m_nodeValues.data(),
                rUpdElems);
        rUpdLogic.m_nodeDirty.reset();

        elemNotified = update_combinational(
                rUpdElems[gc_elemGate].m_localDirty.ones(),
                rCircuit.m_elements.m_perType[gc_elemGate].m_localToElem.data(),
                rCircuit.m_logicNodes.m_elemConnect,
                rCircuit.m_logicValues.m_nodeValues.data(),
                rCircuit.m_gates,
                rUpdLogic);
        rUpdElems[gc_elemGate].m_localDirty.reset();

        steps ++;
    }

    return steps;
}

/**
 * @brief Use "__##__##" strings as waveforms fed into a circuit's inputs and
 *        print output waveforms
 */
static void stupid_scope(
        std::initializer_list<Waveform> inWaves,
        std::initializer_list<NodeId> out,
        UserCircuit& rCircuit,
        UpdateNodes<ELogic>& rUpdLogic,
        UpdateElemTypes_t& rUpdElems,
        int maxSteps)
{
    std::vector<std::string> outWaveforms(out.size());
    for (std::string &rStr : outWaveforms)
    {
        rStr.resize(std::begin(inWaves)->m_wave.size());
    }

    std::size_t minSize = inWaves.begin()->m_wave.size();

    for (Waveform wave : inWaves)
    {
        minSize = std::min(minSize, wave.m_wave.size());
    }

    unsigned int pos = 0;
    for (unsigned int i = 0; i < minSize; ++i)
    {
        for (Waveform wave : inWaves)
        {
            rUpdLogic.assign(wave.m_node, (wave.m_wave[pos] == '#') ? ELogic::High : ELogic::Low);
        }

        step_until_stable(rCircuit, rUpdLogic, rUpdElems, maxSteps);

        for (unsigned int j = 0; j < out.size(); ++j)
        {
            outWaveforms[j][pos] = is_high(rCircuit.m_logicValues.m_nodeValues[*(out.begin() + j)]) ? '#' : '_';
        }

        ++pos;
    }

    for (unsigned int i = 0; i < inWaves.size(); ++i)
    {
        char letter = 'A' + i;
        std::cout << " In[" << letter << "]: " << (inWaves.begin() + i)->m_wave << "\n";
    }

    for (unsigned int i = 0; i < out.size(); ++i)
    {
        char letter = 'A' + i;
        std::cout << "Out[" << letter << "]: " << outWaveforms[i] << "\n";

    }
}

/**
 * @brief Test adding a single XOR gate manually, without using the circuit
 *        builder abstraction
 */
static void test_manual_build()
{
    UserCircuit circuit(64, 64, 2);

    PerElemType &rPerType = circuit.m_elements.m_perType[gc_elemGate];

    // Create Element Id and Local Id
    ElementId const xorElem = circuit.m_elements.m_ids.create();
    ElemLocalId const xorLocal = rPerType.m_localIds.create();

    // Assign Type and Local ID
    rPerType.m_localToElem[xorLocal] = xorElem;
    circuit.m_elements.m_elemTypes[xorElem] = gc_elemGate;
    circuit.m_elements.m_elemToLocal[xorElem] = xorLocal;

    // Assign single gate as combinational XOR
    circuit.m_gates.m_localGates[xorLocal] = { CombinationalGates::Op::XOR, false };

    // create 3 Nodes
    std::array<NodeId, 3> nodes;
    circuit.m_logicNodes.m_nodeIds.create(std::begin(nodes), std::end(nodes));

    // make them easier to access using structured bindings
    auto const [A, B, out] = nodes;

    // Connect 'ports' of XOR gate. Adds a Element->Node mapping
    // For basic gates, first (0) is the output, the rest are inputs
    circuit.m_logicNodes.m_elemConnect.emplace(gc_elemGate, {out, A, B});

    // Connect publishers and subscribers. Adds Node->Element mapping
    circuit.m_logicNodes.m_nodePublisher[out] = xorElem;
    circuit.m_logicNodes.m_nodeSubscribers.emplace(A, {{xorLocal, gc_elemGate}});
    circuit.m_logicNodes.m_nodeSubscribers.emplace(B, {{xorLocal, gc_elemGate}});

    // Now everything is set, circuit can run!

    UpdateElemTypes_t updElems = circuit.setup_element_updater();
    UpdateNodes<ELogic> updLogic = circuit.setup_logic_updater();

    auto const& outVal = circuit.m_logicValues.m_nodeValues[out];

    std::cout << "Single XOR without circuit builder:\n";

    updLogic.assign(A, ELogic::Low);
    updLogic.assign(B, ELogic::Low);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* 0 XOR 0 = " << is_high(outVal) << "\n";

    updLogic.assign(A, ELogic::Low);
    updLogic.assign(B, ELogic::High);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* 0 XOR 1 = " << is_high(outVal) << "\n";

    updLogic.assign(A, ELogic::High);
    updLogic.assign(B, ELogic::Low);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* 1 XOR 0 = " << is_high(outVal) << "\n";

    updLogic.assign(A, ELogic::High);
    updLogic.assign(B, ELogic::High);
    step_until_stable(circuit, updLogic, updElems, 99);
    std::cout << "* 1 XOR 1 = " << is_high(outVal) << "\n";
}

/**
 * @brief Test XOR implementation built with NAND gates
 */
static void test_xor_nand()
{
    UserCircuit circuit(64, 64, 2);

    circuit.build_begin();

    // A >-+------- =NAND1-+ D
    //     |       |       |
    //      =NAND0-+ C      =NAND3--> Out
    //     |       |       |
    // B >-+------- =NAND2-+ E

    auto const [A, B, C, D, E, out] = create_nodes<6, ELogic>();

    gate_NAND({A, B}, C);
    gate_NAND({A, C}, D);
    gate_NAND({C, B}, E);
    gate_NAND({D, E}, out);

    circuit.build_end();

    UpdateElemTypes_t updElems = circuit.setup_element_updater();
    UpdateNodes<ELogic> updLogic = circuit.setup_logic_updater();

    auto const& outVal = circuit.m_logicValues.m_nodeValues[out];

    std::cout << "XOR made from NAND gates:\n";

    updLogic.assign(A, ELogic::Low);
    updLogic.assign(B, ELogic::Low);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* 0 XOR 0 = " << is_high(outVal) << "\n";

    updLogic.assign(A, ELogic::Low);
    updLogic.assign(B, ELogic::High);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* 0 XOR 1 = " << is_high(outVal) << "\n";

    updLogic.assign(A, ELogic::High);
    updLogic.assign(B, ELogic::Low);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* 1 XOR 0 = " << is_high(outVal) << "\n";

    updLogic.assign(A, ELogic::High);
    updLogic.assign(B, ELogic::High);
    step_until_stable(circuit, updLogic, updElems, 99);
    std::cout << "* 1 XOR 1 = " << is_high(outVal) << "\n";
}

/**
 * @brief Test sequential NAND S-R Latch
 */
static void test_sr_latch()
{
    UserCircuit circuit(64, 64, 2);

    circuit.build_begin();

    auto const [Sn, Rn, Q, Qn] = create_nodes<4, ELogic>();

    gate_NAND({Sn, Qn}, Q);
    gate_NAND({Q, Rn}, Qn);

    circuit.build_end();

    UpdateElemTypes_t updElems = circuit.setup_element_updater();
    UpdateNodes<ELogic> updLogic = circuit.setup_logic_updater();

    // initialize to get to valid state
    for (uint32_t elem : circuit.m_elements.m_ids.bitview().zeros())
    {
        ElemTypeId const type = circuit.m_elements.m_elemTypes[elem];
        ElemLocalId const local = circuit.m_elements.m_elemToLocal[elem];
        updElems[type].m_localDirty.set(local);
    }

    std::cout << "NAND SR latch:\n";

    updLogic.assign(Sn, ELogic::Low);
    updLogic.assign(Rn, ELogic::High);
    step_until_stable(circuit, updLogic, updElems, 99);

    auto const& outVal = circuit.m_logicValues.m_nodeValues[Q];

    std::cout << "* set...    Q = " << is_high(outVal) << "\n";

    updLogic.assign(Sn, ELogic::High);
    updLogic.assign(Rn, ELogic::High);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* retain... Q = " << is_high(outVal) << "\n";

    updLogic.assign(Sn, ELogic::High);
    updLogic.assign(Rn, ELogic::Low);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* reset...  Q = " << is_high(outVal) << "\n";

    updLogic.assign(Sn, ELogic::High);
    updLogic.assign(Rn, ELogic::High);
    step_until_stable(circuit, updLogic, updElems, 99);

    std::cout << "* retain... Q = " << is_high(outVal) << "\n";
}

/**
 * @brief Test delay-dependent Edge detector circuit
 */
static void test_edge_detect()
{
    UserCircuit circuit(64, 64, 2);

    circuit.build_begin();

    auto const [A, Dl, Q] = create_nodes<3, ELogic>();

    gate_NAND({A}, Dl);
    gate_AND({A, Dl}, Q);

    circuit.build_end();

    UpdateElemTypes_t updElems = circuit.setup_element_updater();
    UpdateNodes<ELogic> updLogic = circuit.setup_logic_updater();

    // initialize to get to valid state
    for (uint32_t elem : circuit.m_elements.m_ids.bitview().zeros())
    {
        ElemTypeId const type = circuit.m_elements.m_elemTypes[elem];
        ElemLocalId const local = circuit.m_elements.m_elemToLocal[elem];
        updElems[type].m_localDirty.set(local);
    }

    std::cout << "Edge Detector:\n";

    stupid_scope({
        {A, "__##____#___######____#######___"},
    }, {Q}, circuit, updLogic, updElems, 2);
}

int main(int argc, char** argv)
{
    test_manual_build();
    test_xor_nand();
    test_sr_latch();
    test_edge_detect();

    return 0;
}

