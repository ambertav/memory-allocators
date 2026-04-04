#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "common.h"

namespace allocator {

struct Node {
  Node* next;
  size_t size;
};

struct Placement {
  Node* previous;
  Node* current;
  size_t required;
  size_t padding;
};

template <size_t S, BufferType B = BufferType::HEAP,
          FitStrategy F = FitStrategy::FIRST>
class FreeListAllocator {
 public:
  static constexpr BufferType buffer_type = B;
  explicit FreeListAllocator()
    requires(S > 0 && B == BufferType::HEAP);
  explicit FreeListAllocator()
    requires(S > 0 && B == BufferType::STACK);
  explicit FreeListAllocator(std::array<std::byte, S>& buf)
    requires(S > 0 && B == BufferType::EXTERNAL);
  ~FreeListAllocator() noexcept;

  FreeListAllocator(const FreeListAllocator&) = delete;
  FreeListAllocator& operator=(const FreeListAllocator&) = delete;

  FreeListAllocator(FreeListAllocator&&) = delete;
  FreeListAllocator& operator=(FreeListAllocator&&) = delete;

  [[nodiscard]] std::byte* allocate(size_t size, size_t alignment) noexcept;
  void deallocate(std::byte* ptr) noexcept;
  void reset() noexcept;

  std::string get_state() const noexcept;

  size_t get_used() const noexcept;
  size_t get_free() const noexcept;

  //////////////////////
  // type-safe helpers
  //////////////////////
  template <typename T>
  [[nodiscard]] T* allocate(size_t count = 1) noexcept;

  template <typename T>
  void deallocate(T* ptr) noexcept;

  template <typename T, typename... Args>
  [[nodiscard]] T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr) noexcept;

 private:
  Placement find_first_fit(size_t size, size_t alignment) noexcept
    requires(F == FitStrategy::FIRST);

  Placement find_best_fit(size_t size, size_t alignmnet) noexcept
    requires(F == FitStrategy::BEST);

  Node* handle_next_free(Node* current, size_t required_space,
                         size_t remaining) noexcept;
  void handle_links(Node* previous, Node* next) noexcept;

  std::conditional_t<B == BufferType::STACK, std::array<std::byte, S>,
                     std::byte*>
      buffer;
  std::byte* data;
  size_t capacity;
  size_t used;
  Node* head;

  // for get_state()
  std::unordered_map<uintptr_t,
                     std::pair<size_t /* offset */, size_t /* size */>>
      allocations;
};
}  // namespace allocator

#include "free_list_allocator.inl"