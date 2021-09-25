![longeron++](longeron.png) 

*Structure for your framework*

Longeron++ is a C++ library providing ridiculously simple solutions for efficiently representing a program's state, motivated by *Data-Oriented Design*. This library is intended for embedded systems, game engines, desktop applications, and anything large-scale or high-performance.

***This README is just a mockup, and this project does not work yet***

## Introduction

The rise of Data-Oriented Design highlights many unnecessary difficulties associated with Object-Oriented Design. The state of an object-oriented program is often scattered across many complex classes, storing references and calling each other's methods. This is difficult to reason with at large scales. Good object-oriented code is possible, but requires abstractions, loss of performance, and years of experience.

Longeron++ aims to represent the state of the entire program as a single struct at the top-level of the application, consisting of a tree of containers and other structs. A main loop can then pass parts of the top-level struct to different operations in a procedural or functional style. All communication between subsystems is done with asynchronous message passing through queues or similar containers.

## Principles

* No automatic reallocatons
* Lots of colony-style containers?
* Less dynamic allocations

## Features

* None yet lol

## Example Usage

### Entity Component System (ECS)

```cpp
enum class entity : uint32_t { };

// Components for representing different animals
struct cmp_hunger { int m_hunger; };
struct cmp_legs { };
struct cmp_wings { };
struct cmp_fins { };

// lgrn::id_data associates each integer ID with some data (a CMP_T)
// something something maybe sparse set maybe combine two other types idk yet
template<typename CMP_T>
using component_storage_t = lgrn::id_data<entity, CMP_T>;

struct game_state
{
    lgrn::id_registry<entity>       m_entities;

    component_storage_t<cmp_hunger> m_hunger;
    component_storage_t<cmp_legs>   m_legs;
    component_storage_t<cmp_wings>  m_wings;
    component_storage_t<cmp_fins>   m_fins;
}

game_state g_state;

int main()
{
    // setup
    std::array<entity, 3> ents = g_state.m_entities.create(3);

    // ents[0] is a cat, which has hunger and legs
    g_state.m_hunger.emplace(ents[0]);
    g_state.m_legs.emplace(ents[0]);

    // ents[1] is a bird, which has hunger, wings, and legs
    g_state.m_hunger.emplace(ents[1]);
    g_state.m_legs.emplace(ents[1]);
    g_state.m_wings.emplace(ents[1]);

    // ents[2] is a fish, which has hunger and fins
    g_state.m_hunger.emplace(ents[2]);
    g_state.m_wings.emplace(ents[2]);

    while (1)
    {
    	//TODO
    }
}
```

## Notes

* A longeron is one of the main structural elements of an aircraft or similar vehicle used in aerospace.