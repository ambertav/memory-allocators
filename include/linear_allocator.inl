#pragma once

#include <memory>
#include <utility>

#include "linear_allocator.h"

namespace allocator {
template <size_t S, BufferType B>
LinearAllocator<S, B>::LinearAllocator()
  requires(S > 0 && B == BufferType::HEAP)
    : buffer(static_cast<std::byte*>(::operator new(S))),
      data(buffer),
      capacity(S),
      offset(0),
      previous_offset(0) {}

template <size_t S, BufferType B>
LinearAllocator<S, B>::LinearAllocator()
  requires(S > 0 && B == BufferType::STACK)
    : buffer(std::array<std::byte, S>{}),
      data(buffer.data()),
      capacity(S),
      offset(0),
      previous_offset(0) {}

template <size_t S, BufferType B>
LinearAllocator<S, B>::LinearAllocator(std::span<std::byte> buf)
  requires(S > 0 && B == BufferType::EXTERNAL)
    : buffer(buf.data()),
      data(buf.data()),
      capacity(buf.size()),
      offset(0),
      previous_offset(0) {}

template <size_t S, BufferType B>
LinearAllocator<S, B>::~LinearAllocator() noexcept {
  if constexpr (B == BufferType::HEAP) {
    ::operator delete(buffer);
  }
}

template <size_t S, BufferType B>
std::byte* LinearAllocator<S, B>::allocate(size_t size,
                                           size_t alignment) noexcept {
  if (!is_valid_alignment(alignment)) {
    return nullptr;
  }
  size_t aligned{align_forward(offset, alignment)};
  if (aligned < offset) {  // check uint overflow
    return nullptr;
  }

  size_t new_offset{aligned + size};
  if (new_offset > capacity) {
    return nullptr;
  } else {
    previous_offset = aligned;
    offset = new_offset;
    return (data + aligned);
  }
}

template <size_t S, BufferType B>
void LinearAllocator<S, B>::reset() noexcept {
  previous_offset = 0;
  offset = 0;
}

template <size_t S, BufferType B>
std::byte* LinearAllocator<S, B>::resize_last(std::byte* previous_memory,
                                              size_t new_size,
                                              size_t alignment) noexcept {
  if (!is_valid_alignment(alignment)) {
    return nullptr;
  }

  // verify pointer to previous allocation
  size_t previous_aligned{align_forward(previous_offset, alignment)};
  if (data + previous_aligned != previous_memory) {
    return nullptr;
  }

  // check fit
  size_t new_offset = previous_aligned + new_size;
  if (new_offset > capacity) {
    return nullptr;
  }

  // update and return same pointer
  offset = new_offset;
  return previous_memory;
}

//////////////////////
// type-safe helpers
//////////////////////

template <size_t S, BufferType B>
template <typename T>
T* LinearAllocator<S, B>::allocate(size_t count) noexcept {
  if (count > SIZE_MAX / sizeof(T)) {  // check uint overflow
    return nullptr;
  }

  size_t size{sizeof(T) * count};
  size_t alignment{alignof(T)};
  return reinterpret_cast<T*>(allocate(size, alignment));
}

template <size_t S, BufferType B>
template <typename T, typename... Args>
T* LinearAllocator<S, B>::emplace(Args&&... args) {
  size_t size{sizeof(T)};
  size_t alignment{alignof(T)};

  std::byte* ptr{allocate(size, alignment)};
  if (!ptr) {
    return nullptr;
  }

  return std::construct_at(reinterpret_cast<T*>(ptr),
                           std::forward<Args>(args)...);
}

template <size_t S, BufferType B>
template <typename T>
void LinearAllocator<S, B>::destroy(T* ptr) noexcept {
  // asymmetric, does not deallocate (only reset does)
  if (ptr) {
    std::destroy_at(ptr);
  }
}
}  // namespace allocator