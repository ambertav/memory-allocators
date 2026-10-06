#pragma once

#include <cstddef>
#include <mutex>
#include <string>

namespace allocator {
template <typename Allocator>
class Locked {
 public:
  template <typename... Args>
  Locked(Args&&... args);
  ~Locked() noexcept = default;

  Locked(const Locked&) = delete;
  Locked& operator=(const Locked&) = delete;

  Locked(Locked&&) = delete;
  Locked& operator=(Locked&&) = delete;

  template <typename... Args>
  [[nodiscard]] std::byte* allocate(Args&&... args) noexcept;

  template <typename... Args>
  [[nodiscard]] std::byte* resize_last(Args&&... args) noexcept;

  void deallocate(std::byte* ptr) noexcept;
  void reset() noexcept;

  std::string get_state() const noexcept;

  size_t get_used() const noexcept;
  size_t get_free() const noexcept;

  //////////////////////
  // type-safe helpers
  //////////////////////

  template <typename T>
  [[nodiscard]] T* allocate_as() noexcept;

  template <typename T, typename... Args>
  [[nodiscard]] T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr) noexcept;

 private:
  mutable std::mutex mutex;
  Allocator alloc;
};
}  // namespace allocator


#include "locked.inl"