#include <longeron/containers/intarray_multimap.hpp>
#include <longeron/id_management/registry.hpp>

#include <iostream>
#include <vector>

namespace circuits
{

using HierBitset_t      = lgrn::HierarchicalBitset<unsigned long int>;

using ElementId_t       = unsigned int;
using LocalElemId_t     = unsigned int;
using ElemTypeId_t      = unsigned int;

struct Elements
{
    // IDs for each type of elements. i.e. 0 for AND gates, 1 for OR gates
    //lgrn::IdRegistry<ElemTypeId_t, true> m_typeIds;

    struct ElemTypeLocals
    {
        lgrn::IdRegistry<LocalElemId_t, true>   m_ids;
        std::vector<ElementId_t>                m_toElemId;
    };
    std::vector<ElemTypeLocals>     m_typeLocals;


    // IDs for each logic element, regardless of type
    lgrn::IdRegistry<ElementId_t, true> m_elemIds;
    std::vector<ElemTypeId_t>           m_elemTypes;
    std::vector<LocalElemId_t>          m_elemToLocal;

    void resize_elements(std::size_t size)
    {
        m_elemIds.reserve(size);
        m_elemTypes.resize(size, lgrn::id_null<circuits::ElemTypeId_t>());
        m_elemToLocal.resize(size, lgrn::id_null<circuits::LocalElemId_t>());
    }

    void resize_types(std::size_t size)
    {
        m_typeLocals.resize(size);
    }

    void resize_type_elements(ElemTypeId_t type, std::size_t size)
    {
        ElemTypeLocals &rLocals = m_typeLocals.at(type);
        rLocals.m_ids.reserve(size);
        rLocals.m_toElemId.resize(size);
    }
};


template <typename ID_T, typename VALUE_T>
struct SignalNodes
{
    using Subscribers_t = lgrn::IntArrayMultiMap<ID_T, ElementId_t>;

    lgrn::IdRegistry<ID_T, true>    m_ids;
    std::vector<VALUE_T>            m_values;

    Subscribers_t                   m_subscribers;
    std::vector<ElementId_t>        m_publisher;
};

//-----------------------------------------------------------------------------

using UpdateElements_t = std::vector<HierBitset_t>;

template <typename ID_T, typename VALUE_T>
struct NodeChange
{
    ID_T m_node;
    VALUE_T m_value;
};

template <typename ID_T, typename VALUE_T>
using NodeChanges_t = std::vector< NodeChange<ID_T, VALUE_T> >;

template <typename ID_T, typename VALUE_T>
using NodeChangesIt_t = typename NodeChanges_t<ID_T, VALUE_T>::const_iterator;


template <typename ID_T, typename VALUE_T>
void publish(
        NodeChangesIt_t<ID_T, VALUE_T>  first,
        NodeChangesIt_t<ID_T, VALUE_T>  last,
        Elements const&                 elements,
        SignalNodes<ID_T, VALUE_T>&     rNodes,
        UpdateElements_t&               rUpdElem)
{
    using Span_t = typename SignalNodes<ID_T, VALUE_T>::Subscribers_t::Span_t;

    while (first != last)
    {
        NodeChange<ID_T, VALUE_T> const &changes = *first;

        // Set value of node
        rNodes.m_values[changes.m_node] = changes.m_value;

        // Notify connected elements
        Span_t const& subscribers = rNodes.m_subscribers[changes.m_node];

        for (ElementId_t const elemId : subscribers)
        {
            ElemTypeId_t const elemTypeId = elements.m_elemTypes[elemId];
            LocalElemId_t const elemLocalId = elements.m_elemToLocal[elemId];
            rUpdElem[elemTypeId].set(elemLocalId);
        }

        ++ first;
    }
}

template <typename ID_T, typename VALUE_T>
void publish(
        NodeChanges_t<ID_T, VALUE_T>    changes,
        Elements const&                 elements,
        SignalNodes<ID_T, VALUE_T>&     rNodes,
        UpdateElements_t&               rUpdElem)
{
    return publish(std::cbegin(changes), std::cend(changes),
                   elements, rNodes, rUpdElem);
}

//-----------------------------------------------------------------------------

template <typename NODE_ID_T, typename VALUE_T>
struct Signal
{
    SignalNodes<NODE_ID_T, VALUE_T>                 m_nodes;
    lgrn::IntArrayMultiMap<ElementId_t, NODE_ID_T>  m_elemConnect;

    void resize_nodes(std::size_t size)
    {
        m_nodes.m_ids.reserve(size);
        m_nodes.m_publisher.resize(size);
        m_nodes.m_subscribers.resize_ids(size);
        m_nodes.m_values.resize(size);
    }

    void resize_connections(std::size_t size)
    {
        m_elemConnect.resize_data(size);
        m_nodes.m_subscribers.resize_data(size);
    }

    void resize_elements(std::size_t size)
    {
        m_elemConnect.resize_ids(size);
    }
};

template<typename NODE_ID_T, typename VALUE_T>
using calc_func_t = void(*)(
    NODE_ID_T const*                        pConnections,
    int const                               connSize,
    SignalNodes<NODE_ID_T, VALUE_T> const&  nodes,
    NodeChanges_t<NODE_ID_T, VALUE_T>&      rChangesOut);

template< typename NODE_ID_T, typename VALUE_T,
          calc_func_t<NODE_ID_T, VALUE_T> CALC_T >
void calculate_signal_gate(
        HierBitset_t::Iterator              first, // = LocalElemId_t
        HierBitset_t::Iterator              last,
        Signal<NODE_ID_T, VALUE_T> const&   signal,
        std::vector<ElementId_t>            localToElemId,
        NodeChanges_t<NODE_ID_T, VALUE_T>&  rChangesOut)
{
    using Span_t = typename SignalNodes<NODE_ID_T, VALUE_T>::Subscribers_t::Span_t;

    while (first != last)
    {
        ElementId_t const elemId = localToElemId[*first];

        // read connections
        Span_t const &connections = signal.m_elemConnect[elemId];

        CALC_T(connections.begin(), connections.size(),
               signal.m_nodes, rChangesOut);

        ++ first;
    }
}

//-----------------------------------------------------------------------------

struct Connection
{
    ElementId_t m_element;
    int m_port;
};

template <typename NODE_ID_T, typename VALUE_T, typename IT_T>
NODE_ID_T add_node_signal(
        Signal<NODE_ID_T, VALUE_T> &rSignal,
        Connection pub,
        IT_T subFirst, IT_T subLast)
{
    static_assert(std::is_same_v<typename std::iterator_traits<IT_T>::value_type, Connection>);

    NODE_ID_T const nodeId = rSignal.m_nodes.m_ids.create();

    // First connection publishes values (element output)
    ElementId_t const publisher = pub.m_element;
    rSignal.m_nodes.m_publisher[nodeId] = publisher;
    rSignal.m_elemConnect[publisher][pub.m_port] = nodeId;

    // Remaining connections are subscribers (element inputs)
    int const subCount = std::distance(subFirst, subLast);
    ElementId_t* pElemSub
            = rSignal.m_nodes.m_subscribers.emplace(nodeId, subCount);
    IT_T subCurr = subFirst;
    for (int i = 0; i < subCount; i ++)
    {
        ElementId_t const subscriber = subCurr->m_element;
        pElemSub[i] = subscriber;
        rSignal.m_elemConnect[subscriber][subCurr->m_port] = subscriber;
        std::advance(subCurr, 1);
    }

    return nodeId;
}

inline ElementId_t add_element(Elements& rElements, ElemTypeId_t typeId)
{
    ElementId_t const elemId = rElements.m_elemIds.create();
    Elements::ElemTypeLocals &rTypeLocals = rElements.m_typeLocals[typeId];
    LocalElemId_t const localId = rTypeLocals.m_ids.create();

    rTypeLocals.m_toElemId[localId] = elemId;
    rElements.m_elemToLocal[elemId] = localId;

    rElements.m_elemTypes[elemId] = typeId;

    return elemId;
}

template <typename NODE_ID_T, typename VALUE_T>
ElementId_t add_element_signal(
        Signal<NODE_ID_T, VALUE_T> &rSignal,
        Elements& rElements,
        ElemTypeId_t typeId,
        int connections)
{
    ElementId_t const elemId = add_element(rElements, typeId);
    NODE_ID_T* pNodes = rSignal.m_elemConnect.emplace(elemId, connections);
    std::fill_n(pNodes, connections, lgrn::id_null<NODE_ID_T>());
    return elemId;
}

//-----------------------------------------------------------------------------

using NodeLogicId_t = unsigned int;
enum class LogicValue : unsigned char { LOW, HIGH };

using Logic_t = Signal<NodeLogicId_t, LogicValue>;
using LogicNodeChanges_t = NodeChanges_t<NodeLogicId_t, LogicValue>;

//-----------------------------------------------------------------------------



} // namespace circuits

namespace circuitdemo
{

constexpr circuits::ElemTypeId_t const gc_IN    = 0;
constexpr circuits::ElemTypeId_t const gc_OUT   = 1;
constexpr circuits::ElemTypeId_t const gc_AND   = 2;
constexpr circuits::ElemTypeId_t const gc_NAND  = 3;
constexpr circuits::ElemTypeId_t const gc_OR    = 4;
constexpr circuits::ElemTypeId_t const gc_NOR   = 5;
constexpr circuits::ElemTypeId_t const gc_XOR   = 6;
constexpr circuits::ElemTypeId_t const gc_XNOR  = 7;
constexpr circuits::ElemTypeId_t const gc_BUF   = 8;
constexpr circuits::ElemTypeId_t const gc_NOT   = 9;

struct UserCircuit
{
    circuits::Elements  m_elements;
    circuits::Logic_t   m_logic;

    circuits::ElemTypeId_t add_element_signal(
            circuits::ElemTypeId_t typeId, int connections)
    {
        using namespace circuits;
        return circuits::add_element_signal<NodeLogicId_t, LogicValue>(
                    m_logic, m_elements, typeId, connections);
    }

    template <typename IT_T>
    circuits::NodeLogicId_t add_node_logic(
            circuits::Connection publisher, IT_T subFirst, IT_T subLast)
    {
        using namespace circuits;
        return circuits::add_node_signal<NodeLogicId_t, LogicValue, IT_T>(
                    m_logic, publisher, subFirst, subLast);
    }

    circuits::NodeLogicId_t add_node_logic(circuits::Connection publisher, std::initializer_list<circuits::Connection> list)
    {
        return add_node_logic(publisher, std::cbegin(list), std::cend(list));
    }
};

struct UserCircuitUpdater
{
    circuits::UpdateElements_t      m_updElements;
    circuits::LogicNodeChanges_t    m_updLogic;


};

} // namespace circuitdemo

int main(int argc, char** argv)
{
    using namespace circuitdemo;
    using namespace circuits;

    UserCircuit circuit;

    circuit.m_elements.resize_types(8);

    circuit.m_elements.resize_elements(30);
    circuit.m_logic.resize_elements(30);

    circuit.m_logic.resize_connections(16);
    circuit.m_logic.resize_nodes(16);

    circuit.m_elements.resize_type_elements(gc_NAND, 16);

    ElementId_t const nand0 = circuit.add_element_signal(gc_NAND, 3);
    ElementId_t const nand1 = circuit.add_element_signal(gc_NAND, 3);

    NodeLogicId_t const node0 = circuit.add_node_logic({nand1, 0}, {
        {nand0, 1}
    });

    std::cout << "Hullo world\n";
    return 0;
}

