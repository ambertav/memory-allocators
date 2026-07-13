# Pool Allocator

An allocator for fixed-size allocations and deallocations. Pool allocators excel in scenarios involving the frequent allocations and deallocations of many objects of similiar sizes.

## Source
- [Header](../include/pool_allocator.h)
- [Implementation](../include/pool_allocator.inl)

## Design

The `PoolAllocator` divides a contiguous buffer into fixed-size `Chunk`s of `C` bytes and connects them into a singly linked list. On construction, the buffer is partitioned into `S / C` chunks.

Because each `Chunk` is the same size, `allocate()` and `deallocate()` are both O(1). There is no searching, splitting, or coalescing, which avoids external fragmentation and traversal costs.

The allocator allows for a `BufferType` argument, in which the caller can specify the type of memory (heap, stack, or external). `BufferType::STACK` uses a fixed-size array stored inline within the allocator object. `BufferType::EXTERNAL` signals a contract in which the allocator will allocate but not own or manage the memory's lifetime. The size of this external buffer must be known at compile time.  When `BufferType` is not specified, the allocator defaults to `BufferType::HEAP`, dynamically allocating memory and managing the cleanup in its destructor. Hence, the copy, copy assignment, move, and move assignment operations are deleted per the rule of 5.

## Limitations

There is a potential for internal fragmentation, in particular when `sizeof(T)` is less than `C`.

## API Reference

### Constructor
```cpp
template <size_t S, size_t C, BufferType B>
PoolAllocator()
```

Creates a pool allocator with capacity `S` bytes and chunk_size `C` bytes. Behavior depends on `BufferType`:
- `BufferType::HEAP`: Allocates `S` bytes on the heap
- `BufferType::STACK`: Uses a stack-allocated buffer of `S` bytes
- `BufferType::EXTERNAL`: Requires explicit buffer via `PoolAllocator(std::array<std::byte, S>&)`

### Memory Management

```cpp
[[nodiscard]] std::byte* allocate() noexcept
```

Allocates a single chunk of `C` bytes by popping from the head of the underlying linked list of `Chunk`. Returns a pointer to the allocated chunk, or `nullptr` on failure (insufficient space).

```cpp
void deallocate(std::byte* ptr) noexcept
```

Reclaims the allocation at `ptr` to the linked list of `Chunk` without calling a destructor. The `ptr` must have been returned by `allocate()`. Passing `nullptr` returns immediately, with no operation.

```cpp
void reset() noexcept
```

Resets the allocator, reclaiming all memory for reuse. Invalidates all previously allocated pointesr without calling
destructors. For non-trivial types, consider calling `destroy<T>()` before resetting.


### Metrics

```cpp
size_t get_chunk_count() const noexcept
```

Returns the number of chunks with the underlying linked list of `Chunk`.

```cpp
size_t get_used() const noexcept
```

Returns the number of bytes currently allocated.

```cpp
size_t get_free() const noexcept
```

Returns the number of bytes not yet allocated.

### Typed Helpers

```cpp
template <typename T>
  requires(sizeof(T) <= C)
[[nodiscard]] T* allocate() noexcept
```

Typed allocation for objects of type `T`. Requires that `sizeof(T)` is less than or equal to `C` to ensure the allocation fits within a `Chunk`. Returns a typed pointer or `nullptr` on failure.

```cpp
template <typename T>
void deallocate(T* ptr) noexcept
```

Typed deallocation. Reclaims the `ptr` to the underlying linked list of `Chunk` without calling a destructor. For non-trivial types, consider calling `destroy<T>()` before deallocation.

```cpp
template <typename T, typename... Args>
  requires(sizeof(T) <= C)
[[nodiscard]] T* emplace(Args&&... args);
```

Allocates space for type `T` and constructs an object in-place using constructor agruments `args` and `std::construct_at()`. Requires that `sizeof(T)` is less than or equal to `C` to ensure the allocation fits within a `Chunk`. Returns pointer to the constructed object, or `nullptr` if allocation fails.

```cpp
template <typename T>
void destroy(T* ptr) noexcept;
```

Calls destructor on object at `ptr` via `std::destroy_at()`. Only destroys the object and does **not** deallocate memory. Memory can only be reclaimed via `deallocate<T>()` or `reset()`.


## Usage
```cpp

#include "pool_allocator.h"

// Heap-based allocator: 1KB capacity, 64-byte chunks
allocator::PoolAllocator<1024, 64, allocator::BufferType::HEAP> heap_pool{};

// Allocate and construct objects
int* x {heap_pool.emplace<int>(42)};
struct Particle { float x, y, vx, vy; };
Particle* p {heap_pool.emplace<Particle>(0.0f, 0.0f, 1.0f, -1.0f)};

heap_pool.destroy(p);
heap_pool.deallocate(p);

heap_pool.destroy(x);
heap_pool.deallocate(x);

// Stack-based allocator with 32-byte chunks
allocator::PoolAllocator<512, 32, allocator::BufferType::STACK> stack_pool{};

// Raw chunk allocation
std::byte* chunk {stack_pool.allocate()};
stack_pool.deallocate(chunk);

// External buffer
std::array<std::byte, 2048> my_buffer{};
allocator::PoolAllocator<0, 64, allocator::BufferType::EXTERNAL> ext_pool{my_buffer};

// Metrics
size_t chunk_count {heap_pool.get_chunk_count()};
size_t in_use {heap_pool.get_used()};
size_t available {heap_pool.get_free()};
```


**Note:**
- Choose `C` to match or slightly exceed the size of objects you intend to store as to minimize internal fragmentation
- Destroy objects manually if they have non-trivial destructors
- Call `reset()` to reclaim all memory at once
- Designed for workloads with many fixed-size, same-type allocations

## Performance

Run `./bin/perf` for a full overview of performance across all `BufferType` permutations of the `PoolAllocator`, against the [`LinearAllocator`](linear_allocator.md), [`FreeListAllocator`](free_list_allocator.md), [`BuddyAllocator`](buddy_allocator.md), and the standard implementation of `new`.
