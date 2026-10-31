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
  requires requires(Allocator& a, Args&&... args) {
    a.resize_last(std::forward<Args>(args)...);
  }
std::byte* Locked<Allocator>::resize_last(Args&&... args) noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.resize_last(std::forward<Args>(args)...);
}

template <typename Allocator>
void Locked<Allocator>::deallocate(std::byte* ptr) noexcept
  requires requires(Allocator& a, std::byte* p) { a.deallocate(p); }
{
  std::lock_guard<std::mutex> lock(mutex);
  alloc.deallocate(ptr);
}

template <typename Allocator>
void Locked<Allocator>::reset() noexcept
  requires requires(Allocator& a) { a.reset(); }
{
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
template <typename T>
T* Locked<Allocator>::allocate_as() noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  return alloc.template allocate_as<T>();
}

template <typename Allocator>
template <typename T>
  requires requires(Allocator& a, T* p) { a.template deallocate_as<T>(p); }
void Locked<Allocator>::deallocate_as(T* ptr) noexcept {
  std::lock_guard<std::mutex> lock(mutex);
  alloc.template deallocate_as<T>(ptr);
}

template <typename Allocator>
template <typename T, typename... Args>
T* Locked<Allocator>::emplace(Args&&... args) {
  T* ptr{allocate_as<T>()}; // acquires lock
  if (!ptr) return nullptr;

  try {
    return std::construct_at(ptr, std::forward<Args>(args)...);
  } catch (...) {
    if constexpr (requires { deallocate_as<T>(ptr); }) {
      deallocate_as<T>(ptr);    // acquires lock
    }
    throw;
  }
}

template <typename Allocator>
template <typename T>
void Locked<Allocator>::destroy(T* ptr) noexcept {
  if (!ptr) return;
  std::destroy_at(ptr);

  if constexpr (requires { deallocate_as<T>(ptr); }) {
    deallocate_as<T>(ptr);  // acquires locks
  }
}

}  // namespace allocator