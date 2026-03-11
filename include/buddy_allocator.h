#pragma once

#include <array>
#include <bit>
#include <bitset>
#include <cstddef>
#include <span>
#include <type_traits>

#include "common.h"

namespace allocator {

struct Block {
  Block* next;
  size_t level;
};

template <size_t S, BufferType B = BufferType::HEAP>
class BuddyAllocator {
 public:
  static constexpr BufferType buffer_type = B;

  // NOTE: size must be a power of 2
  explicit BuddyAllocator()
    requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::HEAP);
  explicit BuddyAllocator()
    requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::STACK);
  explicit BuddyAllocator(std::span<std::byte, S> buf)
    requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::EXTERNAL);
  ~BuddyAllocator() noexcept;

  BuddyAllocator(const BuddyAllocator&) = delete;
  BuddyAllocator& operator=(const BuddyAllocator&) = delete;

  BuddyAllocator(BuddyAllocator&&) = delete;
  BuddyAllocator& operator=(BuddyAllocator&&) = delete;

  [[nodiscard]] std::byte* allocate(size_t size) noexcept;
  void deallocate(std::byte* ptr) noexcept;
  void reset() noexcept;

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
  Block* get_buddy(Block* block, size_t level) const noexcept;

  std::conditional_t<B == BufferType::STACK, std::array<std::byte, S>,
                     std::byte*>
      buffer{};
  std::byte* data{};
  size_t capacity;

  static constexpr size_t max_level{std::bit_width(S / sizeof(Block)) - 1};
  std::bitset<S / sizeof(Block)> bitmap{};
  std::array<Block*, max_level + 1> free_blocks{};
};
}  // namespace allocator

#include "buddy_allocator.inl"