#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <type_traits>

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
  explicit LinearAllocator(std::span<std::byte, S> buf)
    requires(S > 0 && B == BufferType::EXTERNAL);
  ~LinearAllocator() noexcept;

  LinearAllocator(const LinearAllocator&) = delete;
  LinearAllocator& operator=(const LinearAllocator&) = delete;

  LinearAllocator(LinearAllocator&&) = delete;
  LinearAllocator& operator=(LinearAllocator&&) = delete;

  [[nodiscard]] std::byte* allocate(size_t size, size_t alignment) noexcept;
  void reset() noexcept;

  [[nodiscard]] std::byte* resize_last(std::byte* previous_memory,
                                       size_t new_size,
                                       size_t alignment) noexcept;

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
      buffer{};
  std::byte* data{};
  size_t capacity;
  size_t offset;
  size_t previous_offset;
};
}  // namespace allocator

#include "linear_allocator.inl"