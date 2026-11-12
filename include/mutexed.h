#pragma once

#include <cstddef>
#include <mutex>
#include <string>

namespace allocator {
template <typename Allocator>
class Mutexed {
 public:
  template <typename... Args>
  explicit Mutexed(Args&&... args);
  ~Mutexed() noexcept = default;

  Mutexed(const Mutexed&) = delete;
  Mutexed& operator=(const Mutexed&) = delete;

  Mutexed(Mutexed&&) = delete;
  Mutexed& operator=(Mutexed&&) = delete;

  template <typename... Args>
  [[nodiscard]] std::byte* allocate(Args&&... args) noexcept;

  template <typename... Args>
    requires requires(Allocator& a, Args&&... args) {
      a.resize_last(std::forward<Args>(args)...);
    }
  [[nodiscard]] std::byte* resize_last(Args&&... args) noexcept;

  void deallocate(std::byte* ptr) noexcept
    requires requires(Allocator& a, std::byte* p) { a.deallocate(p); };

  void reset() noexcept
    requires requires(Allocator& a) { a.reset(); };

  std::string get_state() const noexcept;

  size_t get_used() const noexcept;
  size_t get_free() const noexcept;

  //////////////////////
  // type-safe helpers
  //////////////////////

  template <typename T>
  [[nodiscard]] T* allocate_as() noexcept;

  template <typename T>
    requires requires(Allocator& a, T* p) { a.template deallocate_as<T>(p); }
  void deallocate_as(T* ptr) noexcept;

  template <typename T, typename... Args>
  [[nodiscard]] T* emplace(Args&&... args);

  template <typename T>
  void destroy(T* ptr) noexcept;

 private:
  mutable std::mutex mutex;
  Allocator alloc;
};
}  // namespace allocator

#include "mutexed.inl"