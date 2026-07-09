#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "common.h"

namespace allocator {

struct Chunk {
  Chunk* next;
};

template <size_t S, size_t C, BufferType B = BufferType::HEAP>
class PoolAllocator {
 public:
  static constexpr size_t chunk_size = C;
  static constexpr size_t chunk_count = S / C;
  static constexpr BufferType buffer_type = B;

  explicit PoolAllocator()
    requires(S > 0 && S % C == 0 && C >= sizeof(Chunk) &&
             B == BufferType::HEAP);
  explicit PoolAllocator()
    requires(S > 0 && S % C == 0 && C >= sizeof(Chunk) &&
             B == BufferType::STACK);
  explicit PoolAllocator(std::array<std::byte, S>& buf)
    requires(S > 0 && S % C == 0 && C >= sizeof(Chunk) &&
             B == BufferType::EXTERNAL);
  ~PoolAllocator() noexcept;

  PoolAllocator(const PoolAllocator&) = delete;
  PoolAllocator& operator=(const PoolAllocator&) = delete;

  PoolAllocator(PoolAllocator&&) = delete;
  PoolAllocator& operator=(PoolAllocator&&) = delete;

  [[nodiscard]] std::byte* allocate() noexcept;
  void deallocate(std::byte* ptr) noexcept;
  void reset() noexcept;

  std::string get_state() const noexcept;

  size_t get_chunk_count() const noexcept;
  size_t get_used() const noexcept;
  size_t get_free() const noexcept;

  //////////////////////
  // type-safe helpers
  //////////////////////

  template <typename T>
    requires(sizeof(T) <= C)
  [[nodiscard]] T* allocate() noexcept;

  template <typename T, typename... Args>
    requires(sizeof(T) <= C)
  [[nodiscard]] T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr) noexcept;

 private:
  void divide_into_chunks() noexcept;

  std::conditional_t<B == BufferType::STACK, std::array<std::byte, S>,
                     std::byte*>
      buffer;
  std::byte* data;
  size_t capacity;
  size_t used;
  Chunk* head;

  // for get_state()
  std::unordered_set<uintptr_t> allocations;
};
}  // namespace allocator

#include "pool_allocator.inl"