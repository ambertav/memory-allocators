#include <cassert>

#include "buddy_allocator.h"

namespace allocator {
template <size_t S, BufferType B>
BuddyAllocator<S, B>::BuddyAllocator()
  requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::HEAP)
    : buffer(static_cast<std::byte*>(::operator new(S))),
      data(buffer),
      capacity(S),
      used(0) {
  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->previous = nullptr;
  free_blocks[max_level] = block;
  levels[0] = static_cast<uint8_t>(max_level);
}

template <size_t S, BufferType B>
BuddyAllocator<S, B>::BuddyAllocator()
  requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::STACK)
    : buffer(std::array<std::byte, S>{}),
      data(buffer.data()),
      capacity(S),
      used(0) {
  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->previous = nullptr;
  free_blocks[max_level] = block;
  levels[0] = static_cast<uint8_t>(max_level);
}

template <size_t S, BufferType B>
BuddyAllocator<S, B>::BuddyAllocator(std::array<std::byte, S>& buf)
  requires(S > 0 && (S & (S - 1)) == 0 && B == BufferType::EXTERNAL)
    : buffer(buf.data()), data(buf.data()), capacity(buf.size()), used(0) {
  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->previous = nullptr;
  free_blocks[max_level] = block;
  levels[0] = static_cast<uint8_t>(max_level);
}

template <size_t S, BufferType B>
BuddyAllocator<S, B>::~BuddyAllocator() noexcept {
  if constexpr (B == BufferType::HEAP) {
    ::operator delete(buffer);
  }
}

template <size_t S, BufferType B>
std::byte* BuddyAllocator<S, B>::allocate(size_t size) noexcept {
  size_t effective_size{std::bit_ceil(std::max(size, sizeof(Block)))};
  size_t level{std::bit_width(effective_size / sizeof(Block)) - 1};

  if (level > max_level) {
    return nullptr;
  }

  size_t current{level};
  while (free_blocks[current] == nullptr) {
    ++current;
    if (current > max_level) {
      return nullptr;
    }
  }

  Block* block{free_blocks[current]};
  free_blocks[current] = block->next;
  if (free_blocks[current]) {
    free_blocks[current]->previous = nullptr;
  }

  while (current > level) {
    --current;

    Block* buddy{get_buddy(block, current)};
    size_t buddy_index{(reinterpret_cast<std::byte*>(buddy) - data) / sizeof(Block)};
    levels[buddy_index] = static_cast<uint8_t>(current); 

    buddy->next = free_blocks[current];
    buddy->previous = nullptr;

    if (free_blocks[current]) {
      free_blocks[current]->previous = buddy;
    }
    free_blocks[current] = buddy;
  }

  size_t index{(reinterpret_cast<std::byte*>(block) - data) / sizeof(Block)};
  bitmap.set(index);
  levels[index] = static_cast<uint8_t>(level);
  used += (size_t{1} << level) * sizeof(Block);

  return reinterpret_cast<std::byte*>(block);
}

template <size_t S, BufferType B>
void BuddyAllocator<S, B>::deallocate(std::byte* ptr) noexcept {
  if (ptr == nullptr) {
    return;
  }

  assert(ptr >= data && ptr < data + capacity && "pointer is out of bounds");

  Block* block{reinterpret_cast<Block*>(ptr)};
  size_t index{(reinterpret_cast<std::byte*>(block) - data) / sizeof(Block)};
  bitmap.reset(index);

  size_t level{levels[index]};
  used -= (size_t{1} << level) * sizeof(Block);

  while (level < max_level) {
    Block* buddy{get_buddy(block, level)};
    size_t buddy_index{(reinterpret_cast<std::byte*>(buddy) - data) /
                       sizeof(Block)};

    if (bitmap.test(buddy_index) || levels[buddy_index] != level) {
      break;
    }

    unlink(buddy, level);

    if (block > buddy) {
      block = buddy;
    }

    ++level;
  }

  block->next = free_blocks[level];
  block->previous = nullptr;

  if (free_blocks[level]) {
    free_blocks[level]->previous = block;
  }
  free_blocks[level] = block;
}

template <size_t S, BufferType B>
void BuddyAllocator<S, B>::reset() noexcept {
  bitmap.reset();
  free_blocks = {};

  Block* block{reinterpret_cast<Block*>(data)};
  block->next = nullptr;
  block->previous = nullptr;

  free_blocks[max_level] = block;
  levels[0] = static_cast<uint8_t>(max_level);
}

template <size_t S, BufferType B>
size_t BuddyAllocator<S, B>::get_used() noexcept {
  return used;
}

template <size_t S, BufferType B>
size_t BuddyAllocator<S, B>::get_free() noexcept {
  return capacity - used;
}

//////////////////////
// type-safe helpers
//////////////////////

template <size_t S, BufferType B>
template <typename T>
T* BuddyAllocator<S, B>::allocate(size_t count) noexcept {
  if (count > SIZE_MAX / sizeof(T)) {
    return nullptr;
  }

  return reinterpret_cast<T*>(allocate(sizeof(T) * count));
}

template <size_t S, BufferType B>
template <typename T>
void BuddyAllocator<S, B>::deallocate(T* ptr) noexcept {
  deallocate(reinterpret_cast<std::byte*>(ptr));
}

template <size_t S, BufferType B>
template <typename T, typename... Args>
T* BuddyAllocator<S, B>::emplace(Args&&... args) {
  std::byte* ptr{allocate(sizeof(T))};
  if (!ptr) {
    return nullptr;
  }

  return std::construct_at(reinterpret_cast<T*>(ptr),
                           std::forward<Args>(args)...);
}

template <size_t S, BufferType B>
template <typename T>
void BuddyAllocator<S, B>::destroy(T* ptr) noexcept {
  // asymmetric, does not deallocate (only reset does)
  if (ptr) {
    std::destroy_at(ptr);
  }
}

//////////////////////
// helpers
//////////////////////

template <size_t S, BufferType B>
Block* BuddyAllocator<S, B>::get_buddy(Block* block,
                                       size_t level) const noexcept {
  size_t offset{(reinterpret_cast<std::byte*>(block) - data) ^
                (size_t{1} << level) * sizeof(Block)};
  return reinterpret_cast<Block*>(data + offset);
}

template <size_t S, BufferType B>
void BuddyAllocator<S, B>::unlink(Block* block, size_t level) noexcept {
  if (block->previous) {
    block->previous->next = block->next;
  } else {
    free_blocks[level] = block->next;
  }

  if (block->next) { block->next->previous = block->previous; }
}

}  // namespace allocator