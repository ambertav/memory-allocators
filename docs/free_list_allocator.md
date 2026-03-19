# Free List Allocator

An allocator that supports allocations and deallocations with automatic coalescing. Free list allocators excel in scenarios requiring variable-size allocations and deallocations.

## Source
- [Header](../include/free_list_allocator.h)
- [Implementation](../include/free_list_allocator.inl)

## Design

Free list allocator manages memory within a contiguous buffer through the maintenance of a linked list of free memory blocks. Each allocation searches for a free block, splits if necessary, then returns a pointer to the user. On deallocation, the memory is freed, and any free memory blocks are coalesced to reduce fragmentation.

Alignment is handled by inserting padding between the free list block header and the user pointer. The padding value is stored in the `sizeof(size_t)` bytes immediately before the returned pointer. This allows `deallocate()` to recover the header efficiently.

The allocator allows for a `BufferType` argument, in which the caller can specify the type of memory (heap, stack, or external). `BufferType::STACK` uses a fixed-size array stored inline within the allocator object. `BufferType::EXTERNAL` signals a contract in which the allocator will allocate but not own or manage the memory's lifetime. The size of this external buffer must be known at compile time. When `BufferType` is not specified, the allocator defaults `BufferType::HEAP`, dynamically allocating memory and managing the cleanup in its destructor. Hence, the copy, copy assignment, move, and move assignment operations are deleted per the rule of 5.

The free list allocator takes in a `FitStrategy` argument, in which the caller can specify for either a first-fit or best-fit allocation strategy. `FitStrategy::FIRST` selects the first free block large enough to satisify the allocation request, traversing from the head of the linked list, terminating early at the cost of potential fragmentation. `FitStrategy::BEST` traverses the entire list to select the smallest sufficient block, reducing fragmentation at the cost of O(n) allocation. When `FitStrategy` is not specified, the allocator defaults to `FitStrategy::FIRST`.

## Limitations

Each allocation carries a minimum `sizeof(size_t)` bytes for alignment bookkeeping.

## API Reference

### Constructor
```cpp
template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator()
```

Creates a free list allocator with capacity `S` bytes. Behavior depends on `BufferType`:
- `BufferType::HEAP`: Allocates `S` bytes on the heap
- `BufferType::STACK`: Uses a stack-allocated buffer of `S` bytes
- `BufferType::EXTERNAL`: Requires explicit buffer via `FreeListAllocator(std::array<std::byte, S>&)`

### Memory Management

```cpp
[[nodiscard]] std::byte* allocate(size_t size, size_t alignment) noexcept
```

Allocates `size` bytes aligned to `alignment`. Searches the free list using the configured `FitStrategy`, and splits the found block if applicable. Returns a pointer to allocated memory, or `nullptr` on failre (insuffcient space or invalid alignment).

```cpp
void deallocate(std::byte* ptr) noexcept
```

Reclaims the allocation at `ptr` to the free list without calling a destructor. Automatically coalesces with adjacent free blocks to reduce fragmentation. The `ptr` must have been returned by `allocate()`. Passing `nullptr` returns immediately, with no operation.

```cpp
void reset() noexcept
```

Resets the allocator, reclaiming all memory for reuse. Invalidates all previously allocated pointers without calling destructors. For non-trivial types, consider calling `destroy<T>` before resetting.

### Diagnostics

```cpp
size_t get_used() noexcept
```

Returns the number of bytes currently allocated, including alignment padding.

```cpp
size_t get_free() noexcept
```

Returns the number of bytes not yet allocated.

### Typed Helpers

```cpp
template <typename T>
[[nodiscard]] T* allocate(size_t count = 1) noexcept
```

Typed allocation for `count` number of objects of type `T`. Uses `alignof(T)` and `sizeof(T)` to automatically align. Returns a typed pointer or `nullptr` on failure.

```cpp
template <typename T>
void deallocate(T* ptr) noexcept
```

Typed deallocation. Reclaims the `ptr` to the free list without calling a destructor. For non-trivial types, consider calling `destroy<T>` before deallocation.

```cpp
template <typename T, typename... Args>
[[nodiscard]] T* emplace(Args&&... args)
```

Allocates space for type `T` and constructs an object in-place using constructor arguments `args` and `std::construct_at`. Returns pointer to the constructed object, or `nullptr` if allocation fails.

```cpp
template <typename T>
void destroy(T* ptr) noexcept
```

Calls destructor on object at `ptr` via `std::destroy_at`. Only destorys the object and does **not** deallocate memory. Memory can only be reclaimed on
`deallocate<T>(ptr)` or `reset()`.

## Usage
```cpp
#include "free_list_allocator.h"

// Heap-based allocator (1KB) with first-fit strategy
allocator::FreeListAllocator<1024, allocator::BufferType::HEAP, allocator::FitStrategy::FIRST> heap_alloc{};

// Allocate and construct objects
int* x {heap_alloc.emplace<int>(42)};
std::string* s {heap_alloc.emplace<std::string>("hello")};

heap_alloc.destroy(s);
heap_alloc.deallocate(s);

heap_alloc.destroy(x);
heap_alloc.deallocate(x);

// Stack-based allocator with best-fit strategy
allocator::FreeListAllocator<512, allocator::BufferType::STACK, allocator::FitStrategy::BEST> stack_alloc{};

// Raw byte allocation
std::byte* buffer {stack_alloc.allocate(128, 16)};  // 128 bytes, 16-byte aligned
stack_alloc.deallocate(buffer);

// External buffer
std::array<std::byte, 2048> my_buffer{};
allocator::FreeListAllocator<0, allocator::BufferType::EXTERNAL, allocator::FitStrategy::FIRST> ext_alloc{my_buffer};

// Diagnostics
size_t in_use {heap_alloc.get_used()};
size_t available {heap_alloc.get_free()};
```

**Note:**
- Consider `FitStrategy::FIRST` for performance and `FitStrategy::BEST` for minimizing fragmentation
- Destroy objects manually prior to deallocate if they have non-trivial destructors
- Call `reset()` to reclaim all memory at once
- Designed for workloads with dynamic, variable-sized allocations

## Performance

Run `.bin/perf` for a full overview of performance across all `BufferType` and `FitStrategy` permutations of the `FreeListAllocator`, against the [`LinearAllocator`](linear_allocator.md), [`BuddyAllocator`](buddy_allocator.md), and the standard implementation of `new`.