#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "common.h"

namespace allocator {
template <size_t S, BufferType B = BufferType::HEAP>
class LinearAllocator {
 public:
  static constexpr BufferType buffer_type = B;

  explicit LinearAllocator()
    requires(S > 0 && B == BufferType::HEAP);
  explicit LinearAllocator()
    requires(S > 0 && B == BufferType::STACK);
  explicit LinearAllocator(std::array<std::byte, S>& buf)
    requires(S > 0 && B == BufferType::EXTERNAL);
  ~LinearAllocator() noexcept;

  LinearAllocator(const LinearAllocator&) = delete;
  LinearAllocator& operator=(const LinearAllocator&) = delete;

  LinearAllocator(LinearAllocator&&) = delete;
  LinearAllocator& operator=(LinearAllocator&&) = delete;

  [[nodiscard]] std::byte* allocate(size_t size, size_t alignment) noexcept;

  [[nodiscard]] std::byte* resize_last(std::byte* previous_memory,
                                       size_t new_size,
                                       size_t alignment) noexcept;

  void reset() noexcept;

  std::string get_state() const noexcept;

  //////////////////////
  // type-safe helpers
  //////////////////////
  template <typename T>
  [[nodiscard]] T* allocate(size_t count = 1) noexcept;

  template <typename T, typename... Args>
  [[nodiscard]] T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr) noexcept;

 private:
  std::conditional_t<B == BufferType::STACK, std::array<std::byte, S>,
                     std::byte*>
      buffer;
  std::byte* data;
  size_t capacity;
  size_t offset;
  size_t previous_offset;

  // for get_state()
  std::unordered_map<uintptr_t, size_t> allocations;
};
}  // namespace allocator

#include "linear_allocator.inl"