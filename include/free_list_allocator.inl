#pragma once

#include <cassert>
#include <cstdint>

#include "free_list_allocator.h"

namespace allocator {
template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::FreeListAllocator()
  requires(S > 0 && B == BufferType::HEAP)
    : buffer(static_cast<std::byte*>(::operator new(S))),
      data(buffer),
      capacity(S),
      used(0),
      head(reinterpret_cast<Node*>(buffer)) {
  head->size = S - sizeof(Node);
  head->next = nullptr;
}

template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::FreeListAllocator()
  requires(S > 0 && B == BufferType::STACK)
    : buffer(std::array<std::byte, S>{}),
      data(buffer.data()),
      capacity(S),
      used(0),
      head(reinterpret_cast<Node*>(buffer.data())) {
  head->next = nullptr;
  head->size = S - sizeof(Node);
}

template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::FreeListAllocator(std::array<std::byte, S>& buf)
  requires(S > 0 && B == BufferType::EXTERNAL)
    : buffer(buf.data()),
      data(buf.data()),
      capacity(buf.size()),
      used(0),
      head(reinterpret_cast<Node*>(buf.data())) {
  head->next = nullptr;
  head->size = capacity - sizeof(Node);
}

template <size_t S, BufferType B, FitStrategy F>
FreeListAllocator<S, B, F>::~FreeListAllocator() noexcept {
  if constexpr (B == BufferType::HEAP) {
    ::operator delete(buffer);
  }
}

template <size_t S, BufferType B, FitStrategy F>
std::byte* FreeListAllocator<S, B, F>::allocate(size_t size,
                                                size_t alignment) noexcept {
  if (!is_valid_alignment(alignment)) {
    return nullptr;
  }

  Placement placement{};
  if constexpr (F == FitStrategy::FIRST) {
    placement = find_first_fit(size, alignment);
  } else if constexpr (F == FitStrategy::BEST) {
    placement = find_best_fit(size, alignment);
  }

  if (placement.current == nullptr) {
    return nullptr;
  }

  size_t remaining{placement.current->size - placement.required};

  Node* next{
      handle_next_free(placement.current, placement.required, remaining)};
  handle_links(placement.previous, next);

  placement.current->size = placement.required;
  used += placement.required;

  uintptr_t aligned{reinterpret_cast<uintptr_t>(placement.current) +
                    sizeof(Node) + placement.padding};
  // adds padding pointer right before user data
  *reinterpret_cast<size_t*>(aligned - sizeof(size_t)) = placement.padding;

  uintptr_t ptr{
      static_cast<uintptr_t>(aligned - reinterpret_cast<uintptr_t>(data))};
  size_t offset{static_cast<size_t>(
      reinterpret_cast<std::byte*>(placement.current) - data)};
  allocations[ptr] = {offset, placement.required};
  return reinterpret_cast<std::byte*>(aligned);
}

template <size_t S, BufferType B, FitStrategy F>
void FreeListAllocator<S, B, F>::deallocate(std::byte* ptr) noexcept {
  if (!ptr) {
    return;
  }

  assert(ptr >= data && ptr <= data + capacity && "pointer is out of bounds");

  size_t padding{*(reinterpret_cast<size_t*>(ptr - sizeof(size_t)))};
  Node* node{reinterpret_cast<Node*>(ptr - sizeof(Node) - padding)};

  size_t block_size{node->size};

  Node* current{head};
  Node* previous{};

  while (current != nullptr) {
    if (current > node) {
      break;
    }
    previous = current;
    current = current->next;
  }

  std::byte* block_start{reinterpret_cast<std::byte*>(node)};
  std::byte* block_end{block_start + block_size + sizeof(Node)};
  std::byte* current_start{};
  std::byte* previous_end{};

  if (previous) {
    previous_end =
        reinterpret_cast<std::byte*>(previous) + previous->size + sizeof(Node);
  }
  if (current) {
    current_start = reinterpret_cast<std::byte*>(current);
  }

  if (previous_end == block_start && current_start == block_end) {
    previous->size += block_size + sizeof(Node) + current->size + sizeof(Node);
    previous->next = current->next;

  } else if (previous_end == block_start) {
    previous->size += block_size + sizeof(Node);

  } else if (current_start == block_end) {
    node->size = block_size + current->size + sizeof(Node);
    node->next = current->next;
    handle_links(previous, node);

  } else {
    node->size = block_size;
    node->next = current;
    handle_links(previous, node);
  }

  allocations.erase(static_cast<uintptr_t>(ptr - data));
  used -= block_size;
}

template <size_t S, BufferType B, FitStrategy F>
void FreeListAllocator<S, B, F>::reset() noexcept {
  used = 0;

  if constexpr (B == BufferType::STACK) {
    head = reinterpret_cast<Node*>(buffer.data());
  } else {
    head = reinterpret_cast<Node*>(buffer);
  }

  head->size = S - sizeof(Node);
  head->next = nullptr;

  allocations.clear();
}

template <size_t S, BufferType B, FitStrategy F>
std::string FreeListAllocator<S, B, F>::get_state() const noexcept {
  try {
    std::vector<std::pair<uintptr_t, std::pair<size_t, size_t>>> pointers(
        allocations.begin(), allocations.end());
    std::ranges::sort(pointers);

    std::string blocks{};
    for (const auto& [ptr, info] : pointers) {
      const auto& [start, size] = info;
      if (!blocks.empty()) {
        blocks += ",";
      }

      blocks += "{\"ptr\":" + std::to_string(ptr) +
                ",\"offset\":" + std::to_string(start) +
                ",\"size\":" + std::to_string(size) + ",\"status\":\"used\"}";
    }

    Node* node{head};
    while (node != nullptr) {
      size_t start{
          static_cast<size_t>(reinterpret_cast<std::byte*>(node) - data)};

      if (!blocks.empty()) {
        blocks += ",";
      }
      blocks += "{\"ptr\":null,\"offset\":" + std::to_string(start) +
                ",\"size\":" + std::to_string(node->size) +
                ",\"status\":\"free\"}";

      node = node->next;
    }

    return "{\"totalBytes\":" + std::to_string(S) + ",\"blocks\":[" + blocks +
           "],\"metrics\":{\"used\":" + std::to_string(used) +
           ",\"free\":" + std::to_string(S - used) + ",\"fragmentation\":0}}";
           
  } catch (...) {
    return {};
  }
}

template <size_t S, BufferType B, FitStrategy F>
size_t FreeListAllocator<S, B, F>::get_used() const noexcept {
  return used;
}

template <size_t S, BufferType B, FitStrategy F>
size_t FreeListAllocator<S, B, F>::get_free() const noexcept {
  return capacity - used;
}

//////////////////////
// type-safe helpers
//////////////////////

template <size_t S, BufferType B, FitStrategy F>
template <typename T>
T* FreeListAllocator<S, B, F>::allocate(size_t count) noexcept {
  if (count > SIZE_MAX / sizeof(T)) {
    return nullptr;
  }

  size_t size{sizeof(T) * count};
  size_t alignment{alignof(T)};
  return reinterpret_cast<T*>(allocate(size, alignment));
}

template <size_t S, BufferType B, FitStrategy F>
template <typename T>
void FreeListAllocator<S, B, F>::deallocate(T* ptr) noexcept {
  deallocate(reinterpret_cast<std::byte*>(ptr));
}

template <size_t S, BufferType B, FitStrategy F>
template <typename T, typename... Args>
T* FreeListAllocator<S, B, F>::emplace(Args&&... args) {
  size_t size{sizeof(T)};
  size_t alignment{alignof(T)};

  std::byte* ptr{allocate(size, alignment)};
  if (!ptr) {
    return nullptr;
  }

  return std::construct_at(reinterpret_cast<T*>(ptr),
                           std::forward<Args>(args)...);
}

template <size_t S, BufferType B, FitStrategy F>
template <typename T>
void FreeListAllocator<S, B, F>::destroy(T* ptr) noexcept {
  // asymmetric, does not deallocate (only reset does)
  if (ptr) {
    std::destroy_at(ptr);
  }
}

//////////////////////
// helpers
//////////////////////

template <size_t S, BufferType B, FitStrategy F>
Placement FreeListAllocator<S, B, F>::find_first_fit(size_t size,
                                                     size_t alignment) noexcept
  requires(F == FitStrategy::FIRST)
{
  Node* current{head};
  Node* previous{};

  while (current != nullptr) {
    uintptr_t block{reinterpret_cast<uintptr_t>(current) + sizeof(Node) +
                    sizeof(size_t)};
    uintptr_t aligned{align_forward(block, alignment)};
    size_t padding{aligned -
                   (reinterpret_cast<uintptr_t>(current) + sizeof(Node))};
    size_t required{size + padding};

    if (current->size >= required) {
      return Placement{previous, current, required, padding};
    }

    previous = current;
    current = current->next;
  }

  return {nullptr, nullptr, 0, 0};
}

template <size_t S, BufferType B, FitStrategy F>
Placement FreeListAllocator<S, B, F>::find_best_fit(size_t size,
                                                    size_t alignment) noexcept
  requires(F == FitStrategy::BEST)
{
  size_t min_diff{SIZE_MAX};
  Placement best{nullptr, nullptr, 0, 0};

  Node* current{head};
  Node* previous{};

  while (current != nullptr) {
    uintptr_t block{reinterpret_cast<uintptr_t>(current) + sizeof(Node) +
                    sizeof(size_t)};
    uintptr_t aligned{align_forward(block, alignment)};
    size_t padding{aligned -
                   (reinterpret_cast<uintptr_t>(current) + sizeof(Node))};
    size_t required{size + padding};

    if (current->size >= required) {
      size_t diff{current->size - required};

      if (diff == 0) {
        return Placement{previous, current, required, padding};
      }

      if (diff < min_diff) {
        min_diff = diff;

        best.previous = previous;
        best.current = current;
        best.required = required;
        best.padding = padding;
      }
    }

    previous = current;
    current = current->next;
  }

  return best;
}

template <size_t S, BufferType B, FitStrategy F>
Node* FreeListAllocator<S, B, F>::handle_next_free(Node* current,
                                                   size_t required_space,
                                                   size_t remaining) noexcept {
  if (remaining <= sizeof(Node)) {
    return current->next;
  }

  Node* split{reinterpret_cast<Node*>(reinterpret_cast<std::byte*>(current) +
                                      sizeof(Node) + required_space)};

  split->size = remaining - sizeof(Node);
  split->next = current->next;

  return split;
}

template <size_t S, BufferType B, FitStrategy F>
void FreeListAllocator<S, B, F>::handle_links(Node* previous,
                                              Node* next) noexcept {
  if (previous == nullptr) {
    head = next;
  } else {
    previous->next = next;
  }
}

}  // namespace allocator