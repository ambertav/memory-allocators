#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

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
LinearAllocator<S, B>::LinearAllocator(std::array<std::byte, S>& buf)
  requires(S > 0 && B == BufferType::EXTERNAL)
    : buffer(buf.data()), offset(0), previous_offset(0) {
  // ensures buffer pointer is aligned
  data = reinterpret_cast<std::byte*>(align_forward(
      reinterpret_cast<size_t>(buf.data()), alignof(std::max_align_t)));
  capacity = S - (data - buf.data());
}

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
  }

  previous_offset = aligned;
  offset = new_offset;

  allocations[aligned] = size;
  return (data + aligned);
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

  allocations[previous_offset] = new_size;

  // update and return same pointer
  offset = new_offset;
  return previous_memory;
}

template <size_t S, BufferType B>
void LinearAllocator<S, B>::reset() noexcept {
  previous_offset = 0;
  offset = 0;
  allocations.clear();
}

template <size_t S, BufferType B>
std::string LinearAllocator<S, B>::get_state() const noexcept {
  try {
    std::vector<std::pair<uintptr_t, size_t>> pointers(allocations.begin(),
                                                       allocations.end());
    std::ranges::sort(pointers);

    std::string blocks{};
    for (const auto& [start, size] : pointers) {
      if (!blocks.empty()) {
        blocks += ",";
      }
      blocks += "{\"ptr\":" + std::to_string(start) +
                ",\"offset\":" + std::to_string(start) +
                ",\"size\":" + std::to_string(size) + ",\"status\":\"used\"}";
    }

    if (offset < S) {
      if (!blocks.empty()) {
        blocks += ",";
      }
      blocks += "{\"ptr\":null,\"offset\":" + std::to_string(offset) +
                ",\"size\":" + std::to_string(S - offset) +
                ",\"status\":\"free\"}";
    }

    return "{\"totalBytes\":" + std::to_string(S) + ",\"blocks\":[" + blocks +
           "],\"metrics\":{\"used\":" + std::to_string(offset) +
           ",\"free\":" + std::to_string(S - offset) + ",\"fragmentation\":0}}";

  } catch (...) {
    return {};
  }
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