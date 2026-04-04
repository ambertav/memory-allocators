#include <emscripten/bind.h>

#include "buddy_allocator.h"
#include "free_list_allocator.h"
#include "linear_allocator.h"

using namespace allocator;
constexpr size_t SIZE{1024};
constexpr size_t ALIGNMENT{16};

using Linear = LinearAllocator<SIZE, BufferType::HEAP>;
using FreeList = FreeListAllocator<SIZE, BufferType::HEAP, FitStrategy::FIRST>;
using Buddy = BuddyAllocator<SIZE, BufferType::HEAP>;

EMSCRIPTEN_BINDINGS(allocators) {
  emscripten::class_<Linear>("LinearAllocator")
      .constructor<>()
      .function("allocate", emscripten::optional_override(
                                [](Linear& self, size_t size) -> uintptr_t {
                                  return reinterpret_cast<uintptr_t>(
                                      self.allocate(size, ALIGNMENT));
                                }))
      .function("resizeLast",
                emscripten::optional_override([](Linear& self, uintptr_t ptr,
                                                 size_t new_size) -> uintptr_t {
                  return reinterpret_cast<uintptr_t>(self.resize_last(
                      reinterpret_cast<std::byte*>(ptr), new_size, ALIGNMENT));
                }))
      .function("reset", &Linear::reset)
      .function("getState", &Linear::get_state);

  emscripten::class_<FreeList>("FreeListAllocator")
      .constructor<>()
      .function("allocate", emscripten::optional_override(
                                [](FreeList& self, size_t size) -> uintptr_t {
                                  return reinterpret_cast<uintptr_t>(
                                      self.allocate(size, ALIGNMENT));
                                }))
      .function("deallocate", emscripten::optional_override([](FreeList& self,
                                                               uintptr_t ptr) {
                  self.deallocate(reinterpret_cast<std::byte*>(ptr));
                }))
      .function("reset", &FreeList::reset)
      .function("getState", &FreeList::get_state);

  emscripten::class_<Buddy>("BuddyAllocator")
      .constructor<>()
      .function("allocate",
                emscripten::optional_override(
                    [](Buddy& self, size_t size) -> uintptr_t {
                      return reinterpret_cast<uintptr_t>(self.allocate(size));
                    }))
      .function("deallocate",
                emscripten::optional_override([](Buddy& self, uintptr_t ptr) {
                  self.deallocate(reinterpret_cast<std::byte*>(ptr));
                }))
      .function("reset", &Buddy::reset)
      .function("getState", &Buddy::get_state);
}