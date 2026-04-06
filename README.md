# Memory Allocators

Custom memory allocators developed in C++.

<a href="https://ambertav.github.io/memory-allocators/" target="_blank" rel="noopener noreferrer">View Visualizer</a>

## Motivations

This project was a deep dive into memory allocation strategies and their implementation tradeoffs in C++. The visualizer grew out of a desire to make those tradeoffs visible, and served as an opportunity to explore running native C++ on the web via **Emscripten** and **WASM**.

## Dependencies

- C++20
- CMake
- GoogleTest
- Google Benchmark
- Emscripten
- WebAssembly (WASM)

## Design

The library explores the implementation and tradeoffs between speed, flexibility, and fragmentation for three different allocators: linear, free list, and buddy. 

For in-depth technical details, usage examples and API references, please see the following documentation:

- **[Linear Allocator](docs/linear_allocator.md)**
- **[Free List Allocator](docs/free_list_allocator.md)**
- **[Buddy Allocator](docs/buddy_allocator.md)**

### Allocators

The `LinearAllocator` advances a pointer through a buffer on each allocation and reclaims memory with a batch reset. Each operation is completed in O(1) time. There is no per-object deallocation, which is ideal for frame-scoped allocations where objects share a lifetime.

The `FreeListAllocator` sacrifices speed for flexibility by maintaining a linked list of free blocks. Deallocated memory is returned to the list and coalesced with adjacent free blocks in an attempt to minimize fragmentation. The `FreeListAllocator` supports both a first-fit and best-fit placement strategy on allocation, offering more control over the tradeoff betwen speed and flexibility. 

The `BuddyAllocator` manages memory in power-of-two sized blocks across levels of free lists, internally creating a binary tree structure within the fixed buffer. Blocks are paired as "buddy" blocks, allowing for recursive splitting and coalescing, minimizing external fragmentation and enabling O(log n) allocation and deallocation operations.

All three allocators share a common `BufferType` interface, allowing the caller to specify heap, stack, or externally-owned memory. The copy, move, and assignment operations are deleted where required by ownership semantics.


## Visualizer

The visualizer webpage is intended to provide a quick educational overview and demonstration of the nuances and functionality of each allocator implemented in the C++ library. The visualizer is built in standard HTML, CSS, and vanilla JavaScript, which communicates with the native C++ via **Emscripten** bindings and **WASM**. Thus, each control event performed by the user to alter the state of the allocator directly invokes the corresponding method in the C++ library. Each allocator contains a `get_state()` method, which returns a JSON formatted `std::string` that encodes the state of the allocator for use in debugging and/or communicating between the C++ library and the JavaScript.


## Future Updates

- Calculate and report internal and external fragmentation metrics
- Implement a pool allocator
- Implement a TLS allocator

## Acknowledgments

Implementation inspired by [Ginger Bill's Memory Allocation Strategies](https://www.gingerbill.org/series/memory-allocation-strategies/).
