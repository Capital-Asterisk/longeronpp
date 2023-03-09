![# longeron++](longeron.png) 

![Windows](https://github.com/Capital-Asterisk/longeronpp/actions/workflows/windows.yml/badge.svg)
![Linux](https://github.com/Capital-Asterisk/longeronpp/actions/workflows/linux.yml/badge.svg)
![Mac OS X](https://github.com/Capital-Asterisk/longeronpp/actions/workflows/macos.yml/badge.svg)

Longeron++ is a C++17 header-only library that provides simple solutions to help organize a program's state as arrays for more efficient use of memory. Motivated by *Data-Oriented Design*, this library works as a flexible, simple, and more performant alternative to traditional Object-Oriented Design.

**Intended Applications:**

* ECS Game Engines
* Desktop Applications
* Managing index and vertex buffers for GPUs
* Embedded Systems
* anything really.

*Careful, this library is still in early development! API is not stable. Contributions, bug reports, and suggestions are welcome!*

## Introduction

There are problems with how most (C++) software is written. Objects tend to be **individually allocated** structures that are **addressed using a pointer**.

Just to list a few issues:
* Adding new features to the object often requires modifications to the object type itself.
* Pointers are coupled to certain memory location. Extra caution is needed to copy, move, or serialize structures that rely on pointers.
* OS-level allocations (new/malloc) are not known for good performance
* Cache locality is not great, especially with a high object count

Fairly recently, 'Data-Oriented Design' started to gain some popularity (though it still is very obscure).

See: [Data Oriented Design Resources](https://github.com/dbartolini/data-oriented-design)

The rise of Data-Oriented Design highlights many unnecessary difficulties associated with (class-based) Object-Oriented Design. Not only in terms of performance, but also the explosive complexity of class hierarchies, scattered states, deep call stacks, tedious refactoring, tangled spaghetti messes of references everywhere, etc...

In short, object orientation is often not the best tool for your particular problem.

Many of the Data-Oriented Design resources go over the same ideas, such as CPU caches, structure of arrays, and data transformations. Their resulting solutions are simple, performant, and maintainable; however, a few admit to losing flexibility. The only well-known Data-Oriented alternative to 'objects' was realized in the form of the "Entity-Component-System" (ECS) architecture used in very modern game engines.

See: [Entity Component System FAQ](https://github.com/SanderMertens/ecs-faq)

Personally I find that many ECS implementations don't really capture the true essence of what ECS should really be. Treating 'entities' as 'objects' led to many of the same problems in object-oriented. Almost all implementations feature a centralized registry/world class that handles *everything*, which need to be very complex to fit everyone's use cases. Additionally, trying to fit *every* problem around entities and components is a mistake. For example, trying to use [EnTT](https://github.com/skypjack/entt/) for a [Falling-Sand Game](https://wikipedia.org/wiki/Falling-sand_game) simply fell apart as 2D grids are just best represented by 2D arrays, not entities and components.

From working on a [spaceflight simulator](https://github.com/TheOpenSpaceProgram/osp-magnum) (specifically planet terrain mesh) and various circuit simulators, many ECS-like patterns started to emerge. It turns out the core ideas of ECS go far beyond just ECS and game development. This was the beginning of Longeron++.

## Usage and Features

Longeron++ encourages the **heavy use of integer IDs**. They should be preferred over pointers for storing long-term references:
* A single ID can refer to many arrays/containers of different types (aka: Structure of Arrays)
* New arrays/containers can be easily added to associate additional data to IDs
* Containers can reallocate safely without invalidating IDs
* Damn easy to serialize

### Manage Instances

Instead of individually allocating objects and using pointers, use an "Id Registry" to generate IDs. These IDs can be used as an index to a container to assign data.

```cpp
// Id is just an integer.
using Id = uint32_t;

// Enum class as an Id works too, but is not quite ready due to excessive int casts needed
// enum class Id : { };

// STL-style Id Registry
// Internally, it's a single wrapped std::vector<uint64_t> used as a bitset
lgrn::IdRegistryStl<Id> registry;

// Reserve IDs for 64 objects (aka: a single 64-bit int)
registry.reserve(64);

// Allocate data for our potential 64 objects using std::vector
// Might want to use std::optional if constructors/destructors are important
std::vector<Data> data(64);

// Creating new objects is a matter of generating IDs
// All this does is look for set bits in a bitset, and flips them to zero
Id idA = registry.create(); // returns 0
Id idB = registry.create(); // returns 1
Id idC = registry.create(); // returns 2

do_something(data[idA]); // Our objects 'exist' now, do stuff!

registry.remove(idB); // Delete Id 1

int idD = registry.create(); // returns 1, Id has been reused
```

Instead of a vector, any container that associates data to an Id is a working one. Feel free to choose a container that best fits the task:

Who cares about performance, just use a map lol:
```cpp
std::unordered_map<Id, Data> data;
```


Use [EnTT](https://github.com/skypjack/entt/)'s basic_storage. It's pretty cool.
```cpp
entt::basic_storage<Id, Data> data;
```


`lgrn::BitViewIdRegistry` is a mixin around an range of unsigned integers. Anything integer range with a standard `begin()` and `end()` can be used as an Id Registry:

```cpp
// Zeros are taken IDs, ones are free IDs. Initialize all bits to ones
std::array<uint64_t, 2> idBits{~uint64_t(0), ~uint64_t(0)};

auto fixedRegistry = lgrn::bitview_id_reg(idBits, Id{});

Id idA = fixedRegistry.create(); // returns 0
Id idB = fixedRegistry.create(); // returns 1
Id idC = fixedRegistry.create(); // returns 2

// ...
```


### Separate Relationships from Data

It's common to map objects to strings, or to arrange objects in a hierarchy:

```cpp
// Case 1: Relationship in structure
// * Bloats the object

struct ObjectA
{
    std::string m_myName; // name relationship

    ObjectA *m_pParent; // parent relationship
};

// Case 2: Structure coupled to Relationship
// * Difficult to have many simultaneous relationships

struct ObjectB { /* ... */ };

std::map<std::string, ObjectB> m_objects;

// std::shared_ptr can potentially be used, but forces individual allocations and can be difficult to copy.
```

Along with IDs, these relationships can be cleanly separated from any object data.

```cpp
std::map<std::string, Id> m_nameToObject; // :)

std::vector<Id> m_parents; // parent of objId = m_parents[id]
```

### Reference-counting and RAII-like safety without pointers

Reference counting often requires using a class that stores a pointer to the count, and accesses this count during construction and destruction. Due to side effects, overhead, and general rule of avoiding pointers, Longeron++ provides an alternative solution.

`lgrn::IdOwner` wraps a single integer ID type with some safety features:
* Has a deleted copy constructor
* Will assert if destructed or reassigned while containing a value
* Internal state can only be modified by a single friend class specified by template

Safe reference-counted or unique Ids can be implemented by providing functions in this friend class that creates and or destroy IdOwners.

```cpp
lgrn::IdRefCount<Id> refCounts;
using Owner_t = lgrn::IdOwner< Id, lgrn::IdRefCount<Id> >;

Owner_t ownerA;
Owner_t ownerB;

ownerA.has_value(); // returns false
ownerB.has_value(); // returns false

ownerA = refCounts.ref_add( Id(1337) );

ownerA.value();     // returns 1337
ownerB.has_value(); // returns false

std::swap(ownerA, ownerB);

ownerA.has_value(); // returns false
ownerB.value();     // returns 1337

// only refCount is allowed to create new values
ownerA = refCounts.ref_add(ownerB.value());

ownerA.value();     // returns 1337
ownerB.value();     // returns 1337

refCounts.ref_release(std::move(ownerA)); // safely clear value
ownerB = Owner_t{}; // KABOOM!!!! ABORT!!
```

Since IdOwners are [just a single integer internally](https://github.com/Capital-Asterisk/longeronpp/blob/5092a42f2e685da27bcdcbd84eee94a9e641ecf6/src/longeron/id_management/owner.hpp#L63), there is theoretically no overhead in release builds.

### Useful Containers

* **BitView**: Everything is made of bits, so everything is a bitset! Also allows iterating positions of ones or zeros.
  ```cpp
  std::vector<uint8_t> data{0x04, 0x00, 0x00, 0x00};
  lgrn::BitView bits = lgrn::bit_view(data);
  
  bits.test(2); // = true, as data[0] = 4
  
  bits.set(17);
  bits.set(28);
  
  // Iterate ones, outputs 2, 17, 28
  for (std::size_t pos : bits.ones())
  {
      std::cout << "Ones: " << pos << "\n";
  }
  
  bits.reset(); // reset all bits
  
  // Iterate zeros, outputs 0, 1, 2, 3, 4, 5 .... 31
  for (std::size_t pos : bits.zeros())
  {
      std::cout << "Zeros: " << pos << "\n";
  }

  ```
  Bitsets can work great as dirty flags, as bit positions can be used to represent IDs. Iterating ones of a bitset is only slightly slower than iterating an array/vector of integers.

* **HierarchicalBitset (Deprecating soon! It's inefficient, and a non-owning replacement is coming)**: Uses a hierarchy of bit arrays to represent a range of integers with low memory usage ~~and fast iteration speeds~~.
  ```cpp  
  lgrn::HierarchicalBitset bitset(512); // allocate space for 512 bits
  
  bitset.set(69);
  bitset.set(42);
  bitset.set(420);
  
  for (std::size_t bitNum : bitset)
  {  
      // outputs 42, 69, then 420 (ascending order)
      std::cout << bitNum << "\n";
  }
  ```

* **IntArrayMultiMap**: STL-style owning container. Uses internal integer arrays to map IDs (keys) to variable-sized arrays of data. Stored data is contiguous in memory, but can get fragmented when elements are removed.
  ```cpp  
  // use ints as IDs, use float as data. 16 max floats, 4 max IDs
  IntArrayMultiMap<int, float> multimap(16, 4);

  // Emplace some 'partitions' associated with IDs 0 to 2
  multimap.emplace(0, {1.0f, 2.0f});
  multimap.emplace(1, {3.0f, 4.0f, 5.0f});
  multimap.emplace(2, {6.0f, 7.0f, 8.0f, 9.0f});  
  
  // in memory: [1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 7.0f, 8.0f, 9.0f, ... ]
  
  std::cout << multimap[2][2] << "\n"; // prints 8.0f  
    
  // remove 1 and pack
  multimap.erase(1);  
  multimap.pack();  
    
  // in memory: [1.0f, 2.0f, 6.0f, 7.0f, 8.0f, 9.0f, ... ]
  ```
  
  If moving data during packing is a heavy operation, the `pack(n)` function accepts a max number of moves, intended to spread moves across a couple frames.

## Entity Component System

A 'Longeron++ style' ECS goes something like this:
* **Entity**: An integer Id representing the existence of a 'thing'.
* **Component**: Data associated with an Entity, usually a container.
* **System**: A function that operates on entities and components 

A simple way to represent entities and components may resemble this:
```cpp

using Entity = uint32_t;

struct ComponentA { /* ... */ };
struct ComponentB { /* ... */ };

struct World
{
    lgrn::IdRegistryStl<Entity> m_entities;

    std::vector< std::optional<ComponentA> > m_componentAs;
    std::vector< std::optional<ComponentB> > m_componentBs;
}

```


Systems are the trickiest part to get, as it requires fully switching to a different mindset that focuses on how a world should behave as a whole instead of individual objects. In short, **Don't use a 'per-entity' update function.**

Consider some steps needed to update a simple particle system. Note that we don't want to have a `particle.update()` function:

1. Apply gravity to all particle velocities
2. Add all velocities to positions

Systems in ECS are simply just functions and nothing more, and are somewhat analogous to the real world laws of physics: real-life gravity will act on anything with a mass component. Some systems depend on the outputs of other systems; some don't interfere with each other and can be run in parallel.

Users are free to write abstractions to more easily manage systems and components, but [please do that LAST](https://github.com/TheOpenSpaceProgram/osp-magnum/pull/93) and don't shoot your own foot :)

Possible ways to write systems:
```cpp

// Pass the whole world, not recommended as it depends on World; do only out of laziness.
void system_a( World& rWorld )
{
    for (...)
}

// Pass container(s) by reference
void system_b( Container& rComponentAs, Container& rComponentBs )  
{
    for (...)
}

// Also try spans or iterators

// Main loop
while (running)
{
    system_a(world);
    system_b(world.m_componentAs, world.m_componentBs);
}

```

Knowing exactly which components a system will modify will make parallelism nearly trivial.

### Deleting entities

A common problem in ECS is how entities can be deleted along with all their associated components. Other ECS implementations may turn to some form of polymorphism, but of course we'd want to stay with a pure systems approach.

Entities that are going to be deleted can be added to a container, such as a vector or HierarchicalBitset. Individual systems can then delete their corresponding components.

```cpp
// Perhaps make these for each thread that deletes entities and pass them into systems all at once?
lgrn::HierarchicalBitset m_toDelete;

while (running)
{
    // Pass m_toDelete into a system that can request to delete entities
    system_something(world.m_dataA, world.m_toDelete);
  
    // Run delete systems, maybe in parallel?
    system_delete_a(world.m_componentAs, world.m_toDelete);
    system_delete_b(world.m_componentBs, world.m_toDelete);
    
    // Delete the IDs
    for (std::size_t entId : world.m_toDelete)
    {
        world.m_entities.remove(Entity(entId));
    }
    
    // clear deleted entities for next frame
    world.m_toDelete.reset();
}

```


### Systems that create entities

Solution: don't. Entity creation should happen in one place. Systems can return a value to request entities.

```cpp

std::vector<ThingId> newIds;

while (running)
{
    int entitiesRequired = system_create_calculate_required(world.m_componentAs);
    
    if (entitiesRequired != 0)
    {
        newIds.clear();
        newIds.resize(entitiesRequired);  
          
        // create multiple IDs, store in newIds
        world.m_ids.create(std::begin(newIds), std::end(newIds));
          
        // pass in newly created IDs to the next system
        system_create_resume(world.m_componentAs, newIds);
    }
}

```

### Message passing

*Real message passing this time. Dynamic Dispatch is Not Message Passing (DDINMP)*

If you think about it very carefully, entities and components are simply persistent messages passed between systems. For example, entities and components that describe a solid cube is a way for a physics system to communicate with a render system. 

Passing data structures or components to system functions is an excellent alternative to OOP-style event listeners, signals, or observers.

```cpp
std::vector<SomeEvent> events; // can be dirty flags, commands, or really anything
system_a(world, events); // writes

// pass events into other systems (likely parallelizable)
system_b(world, events);
system_c(world, events);
system_d(world, events);
```

### Centralized world?

Components containers can be easily stored in separate structures to isolate unrelated systems.

```cpp

struct World
{
    lgrn::IdRegistryStl<Entity> m_entities;

    std::vector< std::optional<ComponentA> > m_componentAs;
    std::vector< std::optional<ComponentA> > m_componentBs;
}

// Separate the entire renderer, ez.
struct WorldRendererOpenGL
{
    std::vector< std::optional<GlTransform> > m_transforms;
    std::vector< std::optional<GlTexture> > m_texture;
}

// :)
struct WorldRendererVulkan { ... };
struct WorldRendererDirectX { ... };

// ...

main_render_function_gl(world, worldRendererGl);

```

## Examples

* [Longeron Circuit Simulator](examples/circuits): see other readme

*More examples coming soon: a falling sand game? a voxel game?*

### OSP-Magnum

OSP-Magnum is a work-in-progress spaceflight simulator that Longeron++ originated from. It uses EnTT and other 3rd party libraries.

[OSP-Magnum](https://github.com/TheOpenSpaceProgram/osp-magnum)

Specific files that may be worth looking through:

* [Engine Test Scene](https://github.com/TheOpenSpaceProgram/osp-magnum/blob/edc5dec6d906d8912546b87cb326930b4e2ddd56/src/test_application/activescenes/scenarios_enginetest.cpp): Displays a spinning cube. Note how the rendering functions on the bottom half of the file accept the EngineTestScene as const. The scene is kept as raw data, allowing the renderer to be destructed and recreated without bothering the main scene.
* [Resources Class](https://github.com/TheOpenSpaceProgram/osp-magnum/blob/edc5dec6d906d8912546b87cb326930b4e2ddd56/src/osp/Resource/resources.h): see [Unit Test](https://github.com/TheOpenSpaceProgram/osp-magnum/blob/edc5dec6d906d8912546b87cb326930b4e2ddd56/test/resources/main.cpp) and [Explanation](https://github.com/TheOpenSpaceProgram/osp-magnum/pull/174). The Resources class can store arbitrary types associated to Resource IDs, which feature reference counting using `lgrn::IdOwner`.

## Notes

* A longeron is one of the main structural elements of an aircraft or similar vehicle used in aerospace. This relates to the project's roots in OSP-Magnum, which is a spaceflight simulator.
