# Buddy Allocator

An allocator that supports power-of-two allocations and deallocations with automatic coalescing. Buddy allocators excel in scenarios requiring fast, deterministic allocation with low fragmentation overhead.

## Source
- [Header](../include/buddy_allocator.h)
- [Implementation](../include/buddy_allocator.inl)

## Design

Buddy allocator manages memory within a contiguous buffer by maintaining a set of free lists, one per level, where each level corresponds to a power-of-two block size. The minimum block size is dependent on `sizeof(Block)`, which stores the doubly linked list pointers used to maintain the free lists.

On allocation, the requested size is rounded up to the nearest power of two. If no block exists at the required level, a larger block is split into two buddies, with one inserted into the free list and the other used to satisfy the request. On deallocation, the block is returned to its free list and recursively coalesced with its buddy, reducing fragmentation.

Block levels are tracked in a flat `levels` array indexed by minimum-block offset, and a bitmap tracks which blocks are currently allocated, allowing O(1) buddy lookup and validity checking while coalescing.

The allocator allows for a `BufferType` argument, in which the caller can specify the type of memory (heap, stack, or external). `BufferType::STACK` uses a fixed-size array stored inline within the allocator object. `BufferType::EXTERNAL` signals a contract in which the allocator will allocate but not own or manage the memory's lifetime. The size of this external buffer must be known at compile time. When `BufferType` is not specified, the allocator defaults to `BufferType::HEAP`, dynamically allocating memory and managing cleanup in its destructor. Hence, the copy, copy assignment, move, and move assignment operations are deleted per the rule of 5.

## Limitations

All allocations are rounded up to the nearest power of two, which may cause internal fragmentation for non-power-of-two allocation sizes. The minimum allocation size is `sizeof(Block)`, as the block metadata is stored within the free memory itself. The total capacity `S` must be a power of two greater than zero.

## API Reference

### Constructor

```cpp
template <size_t S, BufferType B>
BuddyAllocator()
```

Creates a buddy allocator with capacity `S` bytes. Behavior depends on `BufferType`:
- `BufferType::HEAP`: Allocates `S` bytes on the heap
- `BufferType::STACK`: Uses a stack-allocated buffer of `S` bytes
- `BufferType::EXTERNAL`: Requires explicit buffer via `BuddyAllocator(std::array<std::byte, S>&)`

### Memory Management

```cpp
[[nodiscard]] std::byte* allocate(size_t size) noexcept
```

Allocates a block of at least `size` bytes, rounded up to the nearest power of two. Searches the free lists and splits larger blocks as needed. Returns a pointer to allocated memory, or `nullptr` on failure (insufficient space).

```cpp
void deallocate(std::byte* ptr) noexcept
```

Reclaims the allocation at `ptr` without calling a destructor. Automatically coalesces with the buddy block if available, recursively merging upward as much as possible. The `ptr` must have been returned by `allocate()`. Passing `nullptr` returns immediately with no operation.

```cpp
void reset() noexcept
```

Resets the allocator, reclaiming all memory for reuse. Invalidates all previously allocated pointers without calling destructors. For non-trivial types, consider calling `destroy<T>` before resetting.

### Diagnostics

```cpp
size_t get_used() noexcept
```

Returns the number of bytes currently allocated.

```cpp
size_t get_free() noexcept
```

Returns the number of bytes not yet allocated.

### Typed Helpers

```cpp
template <typename T>
[[nodiscard]] T* allocate(size_t count = 1) noexcept
```

Typed allocation for `count` number of objects of type `T`. Aligns to `count * sizeof(T)`, rounded up to the nearest power of two. Returns a typed pointer or `nullptr` on failure.

```cpp
template <typename T>
void deallocate(T* ptr) noexcept
```

Typed deallocation. Reclaims `ptr` without calling a destructor. For non-trivial types, consider calling `destroy<T>` before deallocation.

```cpp
template <typename T, typename... Args>
[[nodiscard]] T* emplace(Args&&... args)
```

Allocates space for type `T` and constructs an object in-place using constructor arguments `args` and `std::construct_at`. Returns a pointer to the constructed object, or `nullptr` if allocation fails.

```cpp
template <typename T>
void destroy(T* ptr) noexcept
```

Calls destructor on the object at `ptr` via `std::destroy_at`. Only destorys the object and does **not** deallocate memory. Memory can only be reclaimed via `deallocate<T>(ptr)` or `reset()`.

## Usage

```cpp
#include "buddy_allocator.h"

// Heap-based allocator (1KB)
allocator::BuddyAllocator<1024, allocator::BufferType::HEAP> heap_alloc{};

// Allocate and construct objects
int* x {heap_alloc.emplace<int>(42)};
std::string* s {heap_alloc.emplace<std::string>("hello")};

heap_alloc.destroy(s);
heap_alloc.deallocate(s);

heap_alloc.destroy(x);
heap_alloc.deallocate(x);

// Stack-based allocator
allocator::BuddyAllocator<512, allocator::BufferType::STACK> stack_alloc{};

// Raw byte allocation
std::byte* buffer {stack_alloc.allocate(128)};  // rounded up to 128 bytes
stack_alloc.deallocate(buffer);

// External buffer
std::array<std::byte, 2048> my_buffer{};
allocator::BuddyAllocator<2048, allocator::BufferType::EXTERNAL> ext_alloc{my_buffer};

// Diagnostics
size_t in_use {heap_alloc.get_used()};
size_t available {heap_alloc.get_free()};
```

**Note:**
- Requesting sizes equal or close to a power-of-two boundary minimize internal fragmentation
- Destroy objects manually prior to deallocate if they have non-trivial destructors
- Call `reset()` to reclaim all memory at once
- `S` must be a power of two greater than zero
- Designed for workloads requiring deterministic allocation time and low fragmentation

## Performance

Run `.bin/perf` for a full overview of performance across all `BufferType` permutations of the `BuddyAllocator`, against the [`LinearAllocator`](linear_allocator.md), [`FreeListAllocator`](free_list_allocator.md), and the standard implementation of `new`.