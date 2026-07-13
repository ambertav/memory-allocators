#pragma once

#include <algorithm>
#include <cassert>
#include <ranges>
#include <vector>

#include "pool_allocator.h"

namespace allocator {
template <size_t S, size_t C, BufferType B>
PoolAllocator<S, C, B>::PoolAllocator()
  requires(S > 0 && S % C == 0 && C >= sizeof(Chunk) && B == BufferType::HEAP)
    : buffer(static_cast<std::byte*>(::operator new(S))),
      data(buffer),
      capacity(S),
      used(0),
      head(reinterpret_cast<Chunk*>(buffer)) {
  divide_into_chunks();
}

template <size_t S, size_t C, BufferType B>
PoolAllocator<S, C, B>::PoolAllocator()
  requires(S > 0 && S % C == 0 && C >= sizeof(Chunk) && B == BufferType::STACK)
    : buffer(std::array<std::byte, S>{}),
      data(buffer.data()),
      capacity(S),
      used(0),
      head(reinterpret_cast<Chunk*>(buffer.data())) {
  divide_into_chunks();
}

template <size_t S, size_t C, BufferType B>
PoolAllocator<S, C, B>::PoolAllocator(std::array<std::byte, S>& buf)
  requires(S > 0 && S % C == 0 && C >= sizeof(Chunk) &&
           B == BufferType::EXTERNAL)
    : buffer(buf.data()),
      data(buf.data()),
      capacity(buf.size()),
      used(0),
      head(reinterpret_cast<Chunk*>(buf.data())) {
  divide_into_chunks();
}

template <size_t S, size_t C, BufferType B>
PoolAllocator<S, C, B>::~PoolAllocator() noexcept {
  if constexpr (B == BufferType::HEAP) {
    ::operator delete(buffer);
  }
}

template <size_t S, size_t C, BufferType B>
std::byte* PoolAllocator<S, C, B>::allocate() noexcept {
  if (head == nullptr) {
    return nullptr;
  }

  Chunk* chunk{head};
  head = head->next;
  ++used;
  allocations.insert(reinterpret_cast<uintptr_t>(chunk));

  return reinterpret_cast<std::byte*>(chunk);
}

template <size_t S, size_t C, BufferType B>
void PoolAllocator<S, C, B>::deallocate(std::byte* ptr) noexcept {
  if (!ptr) {
    return;
  }
  assert(ptr >= data && ptr <= data + capacity && "pointer is out of bounds");

  Chunk* chunk{reinterpret_cast<Chunk*>(ptr)};
  uintptr_t address{reinterpret_cast<uintptr_t>(ptr)};

  chunk->next = head;
  head = chunk;
  --used;
  allocations.erase(address);
}

template <size_t S, size_t C, BufferType B>
void PoolAllocator<S, C, B>::reset() noexcept {
  used = 0;

  if constexpr (B == BufferType::STACK) {
    head = reinterpret_cast<Chunk*>(buffer.data());
  } else {
    head = reinterpret_cast<Chunk*>(buffer);
  }

  divide_into_chunks();
  allocations.clear();
}

template <size_t S, size_t C, BufferType B>
std::string PoolAllocator<S, C, B>::get_state() const noexcept {
  try {
    std::vector<uintptr_t> pointers(allocations.begin(), allocations.end());
    std::ranges::sort(pointers);

    std::string blocks{};
    for (const auto& ptr : pointers) {
      size_t offset{
          static_cast<size_t>(reinterpret_cast<std::byte*>(ptr) - data)};

      if (!blocks.empty()) {
        blocks += ",";
      }

      blocks += "{\"ptr\":" + std::to_string(ptr) +
                ",\"offset\":" + std::to_string(offset) +
                ",\"size\":" + std::to_string(C) +
                ",\"header\":0,\"status\":\"used\"}";
    }

    Chunk* chunk{head};
    while (chunk != nullptr) {
      size_t offset{
          static_cast<size_t>(reinterpret_cast<std::byte*>(chunk) - data)};

      if (!blocks.empty()) {
        blocks += ",";
      }

      blocks += "{\"ptr\":null,\"offset\":" + std::to_string(offset) +
                ",\"size\":" + std::to_string(C - sizeof(Chunk)) +
                ",\"header\":" + std::to_string(sizeof(Chunk)) +
                ",\"status\":\"free\"}";

      chunk = chunk->next;
    }

    return "{\"totalBytes\":" + std::to_string(S) + ",\"blocks\":[" + blocks +
           "],\"metrics\":{\"used\":" + std::to_string(get_used()) +
           ",\"free\":" + std::to_string(get_free()) + ",\"fragmentation\":0}}";

  } catch (...) {
    return {};
  }
}

template <size_t S, size_t C, BufferType B>
size_t PoolAllocator<S, C, B>::get_chunk_count() const noexcept {
  return chunk_count;
}

template <size_t S, size_t C, BufferType B>
size_t PoolAllocator<S, C, B>::get_used() const noexcept {
  return used * C;
}

template <size_t S, size_t C, BufferType B>
size_t PoolAllocator<S, C, B>::get_free() const noexcept {
  return (chunk_count - used) * C;
}

//////////////////////
// type-safe helpers
//////////////////////

template <size_t S, size_t C, BufferType B>
template <typename T>
  requires(sizeof(T) <= C)
T* PoolAllocator<S, C, B>::allocate() noexcept {
  return reinterpret_cast<T*>(this->PoolAllocator<S, C, B>::allocate());
}

template <size_t S, size_t C, BufferType B>
template <typename T>
void PoolAllocator<S, C, B>::deallocate(T* ptr) noexcept {
  deallocate(reinterpret_cast<std::byte*>(ptr));
}

template <size_t S, size_t C, BufferType B>
template <typename T, typename... Args>
  requires(sizeof(T) <= C)
T* PoolAllocator<S, C, B>::emplace(Args&&... args) {
  std::byte* ptr{allocate()};
  if (!ptr) {
    return nullptr;
  }

  return std::construct_at(reinterpret_cast<T*>(ptr),
                           std::forward<Args>(args)...);
}

template <size_t S, size_t C, BufferType B>
template <typename T>
void PoolAllocator<S, C, B>::destroy(T* ptr) noexcept {
  // asymmetric, does not deallocate (only reset does)
  if (ptr) {
    std::destroy_at(ptr);
  }
}

//////////////////////
// helpers
//////////////////////

template <size_t S, size_t C, BufferType B>
void PoolAllocator<S, C, B>::divide_into_chunks() noexcept {
  for (size_t i{}; i < chunk_count - 1; ++i) {
    Chunk* current{reinterpret_cast<Chunk*>(data + (i * C))};
    Chunk* next{reinterpret_cast<Chunk*>(data + ((i + 1) * C))};
    current->next = next;
  }

  reinterpret_cast<Chunk*>(data + (chunk_count - 1) * C)->next = nullptr;
}
}  // namespace allocator