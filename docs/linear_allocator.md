# Linear Allocator

A bump-pointer allocator for sequential memory allocation. Linear allocators excel in scenarios with many small, sequential allocations, where individual deallocation is not necessary.

## Source
- [Header](../include/linear_allocator.h)
- [Implementation](../include/linear_allocator.inl)

## Design

Linear allocator is a fast allocator that allots memory by incrementing a pointer through a contiguous, fixed memory buffer. Each allocation advances the pointer forward, making the process O(1) with minimal overhead.

The implementation holds true to the philosophy of linear allocators: interim memory cannot be deallocated or resized. Further, note that the typed helpers `emplace<T>` and `destroy<T>` are asymmetric. `emplace<T>` allocates, constructs, and returns a pointer to the resource, while `destroy<T>` simply destructs the resource. This is purposeful, as `reset()` remains the only way through which to deallocate memory within the linear allocator. 

The allocator allows for a `BufferType` argument, in which the caller can specify the type of memory (heap, stack, or external). `BufferType::STACK` uses a fixed-size array stored inline within the allocator object. `BufferType::EXTERNAL` signals a contract in which the allocator will allocate but not own or manage the memory's lifetime. The size of this external buffer must be known at compile time.  When `BufferType` is not specified, the allocator defaults `BufferType::HEAP`, dynamically allocating memory and managing the cleanup in its destructor. Hence, the copy, copy assignment, move, and move assignment operations are deleted per the rule of 5.


## API Reference

### Constructor
```cpp
template <size_t S, BufferType B>
LinearAllocator()
```

Creates a linear allocator with capacity `S` bytes. Behavior depends on `BufferType`:
- `BufferType::HEAP`: Allocates `S` bytes on the heap
- `BufferType::STACK`: Uses a stack-allocated buffer of `S` bytes
- `BufferType::EXTERNAL`: Requires explicit buffer via `LinearAllocator(std::array<std::byte, S>&)`


### Memory Management
```cpp
[[nodiscard]] std::byte* allocate(size_t size, size_t alignment) noexcept
```

Allocates `size` bytes aligned to `alignment` boundary. Returns pointer to allocated memory, or `nullptr` on failure (insufficient space, invalid alignment, or overflow).

```cpp
[[nodiscard]] std::byte* resize_last(std::byte* previous_memory,
                                       size_t new_size, size_t alignment) noexcept
```

Resizes the most recent allocation, if possible. Returns the same pointer if successful, `nullptr` if the pointer doesn't match the last allocation or if insufficient space remains. Only works on the most recent allocation made.

```cpp
void reset() noexcept
```

Resets the allocator, reclaiming all allocated memory for reuse. Invalidates all previously allocated pointers without calling destructors. For non-trivial types, consider calling `destroy<T>` before resetting. 

### Typed Helpers
```cpp
template <typename T>
[[nodiscard]] T* allocate(size_t count = 1) noexcept
```

Typed allocation for `count` number of objects of type `T`. Uses `alignof(T)` and `sizeof(T)` to automatically align. Returns a typed pointer or `nullptr` on failure.

```cpp
template <typename T, typename... Args>
[[nodiscard]] T* emplace(Args&&... args)
```

Allocates space for type `T` and constructs an object in-place using constructor arguments `args` and `std::construct_at`. Returns pointer to the constructed object, or `nullptr` if allocation fails.

```cpp
template <typename T> 
void destroy(T* ptr) noexcept
```

Calls destructor on object at `ptr` via `std::destroy_at`. Only destroys the object and does **not** deallocate memory. Memory can only be reclaimed on `reset()`.

## Usage
```cpp
#include "linear_allocator.h"

// Heap-based allocator (1KB)
allocator::LinearAllocator<1024, allocator::BufferType::HEAP> heap_alloc{};

// Allocate and construct objects
int* x {heap_alloc.emplace<int>(42)};
std::string* s {heap_alloc.emplace<std::string>("hello")};

// Manual destruction before reset
heap_alloc.destroy(s);
heap_alloc.destroy(x);

// Reclaim all memory
heap_alloc.reset();

// Stack-based allocator (512 bytes)
allocator::LinearAllocator<512, allocator::BufferType::STACK> stack_alloc;

// Raw byte allocation
std::byte* buffer {stack_alloc.allocate(128, 16)};  // 128 bytes, 16-byte aligned

// External buffer
std::array my_buffer{};
allocator::LinearAllocator<2048, allocator::BufferType::EXTERNAL> ext_alloc{my_buffer};
```

**Note:**
- Allocate many objects sequentially
- Destroy objects manually if they have non-trivial destructors
- Call `reset()` to reclaim all memory at once
- Designed for batch workloads, not individual deallocation

## Performance

Run `.bin/perf` for a full overview of performance across all `BufferType` permutations of the `LinearAllocator`, against the [`FreeListAllocator`](free_list_allocator.md), [`BuddyAllocator`](buddy_allocator.md), and the standard implementation of `new`.

