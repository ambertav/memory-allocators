#pragma once

#include <array>
#include <bit>
#include <bitset>
#include <cstddef>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "common.h"

namespace allocator {

struct Block {
  Block* next;
  Block* previous;
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
  explicit BuddyAllocator(std::array<std::byte, S>& buf)
    requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::EXTERNAL);
  ~BuddyAllocator() noexcept;

  BuddyAllocator(const BuddyAllocator&) = delete;
  BuddyAllocator& operator=(const BuddyAllocator&) = delete;

  BuddyAllocator(BuddyAllocator&&) = delete;
  BuddyAllocator& operator=(BuddyAllocator&&) = delete;

  [[nodiscard]] std::byte* allocate(size_t size) noexcept;
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
  Block* get_buddy(Block* block, size_t level) const noexcept;
  void unlink(Block* block, size_t level) noexcept;

  std::conditional_t<B == BufferType::STACK, std::array<std::byte, S>,
                     std::byte*>
      buffer;
  std::byte* data;
  size_t capacity;
  size_t used;

  static constexpr size_t max_level{std::bit_width(S / sizeof(Block)) - 1};
  std::array<Block*, max_level + 1> free_blocks{};
  std::bitset<S / sizeof(Block)> bitmap{};
  std::array<uint8_t, S / sizeof(Block)> levels;

  // for get_state()
  std::unordered_map<uintptr_t, size_t> allocations;
};
}  // namespace allocator

#include "buddy_allocator.inl"