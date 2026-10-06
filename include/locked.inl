#include <mutex>
#include <utility>

#include "locked.h"

namespace allocator {
template <typename Allocator>
template <typename... Args>
Locked<Allocator>::Locked(Args&&... args)
    : alloc(std::forward<Args>(args)...) {}

template <typename Allocator>
template <typename... Args>
std::byte* Locked<Allocator>::allocate(Args&&... args) noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.allocate(std::forward<Args>(args)...);
}

template <typename Allocator>
template <typename... Args>
std::byte* Locked<Allocator>::resize_last(Args&&... args) noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.resize_last(std::forward<Args>(args)...);
}

template <typename Allocator>
void Locked<Allocator>::deallocate(std::byte* ptr) noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  alloc.deallocate(ptr);
}

template <typename Allocator>
void Locked<Allocator>::reset() noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  alloc.reset();
}

template <typename Allocator>
std::string Locked<Allocator>::get_state() const noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.get_state();
}

template <typename Allocator>
size_t Locked<Allocator>::get_used() const noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.get_used();
}

template <typename Allocator>
size_t Locked<Allocator>::get_free() const noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.get_free();
}

//////////////////////
// type-safe helpers
//////////////////////

template <typename Allocator>
template <typename T, typename... Args>
T* Locked<Allocator>::allocate_as(Args&&... args) noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.allocate_as(std::forward<Args>(args)...);
}

template <typename Allocator>
template <typename T, typename... Args>
T* Locked<Allocator>::emplace(Args&&... args) {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.emplace(std::forward<Args>(args)...);
}

template <typename Allocator>
template <typename T>
void Locked<Allocator>::destroy(T* ptr) noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  alloc.destroy(ptr);
}

}  // namespace allocator