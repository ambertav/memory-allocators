#include <emscripten/bind.h>

#include "buddy_allocator.h"
#include "free_list_allocator.h"
#include "linear_allocator.h"
#include "pool_allocator.h"

using namespace allocator;
constexpr size_t SIZE{1024};
constexpr size_t CHUNK_SIZE{128};
constexpr size_t ALIGNMENT{sizeof(Node)};

using Linear = LinearAllocator<SIZE, BufferType::HEAP>;
using Pool = PoolAllocator<SIZE, CHUNK_SIZE, BufferType::HEAP>;
using FreeList = FreeListAllocator<SIZE, FitStrategy::FIRST, BufferType::HEAP>;
using Buddy = BuddyAllocator<SIZE, BufferType::HEAP>;

EMSCRIPTEN_BINDINGS(allocators) {
  emscripten::class_<Linear>("LinearAllocator")
      .constructor<>()
      .function(
          "allocate",
          +[](Linear& self, size_t size) -> uintptr_t {
            return reinterpret_cast<uintptr_t>(self.allocate(size, ALIGNMENT));
          })
      .function(
          "resizeLast",
          +[](Linear& self, uintptr_t ptr, size_t new_size) -> uintptr_t {
            return reinterpret_cast<uintptr_t>(self.resize_last(
                reinterpret_cast<std::byte*>(ptr), new_size, ALIGNMENT));
          })
      .function("reset", &Linear::reset)
      .function("getState", &Linear::get_state);

  emscripten::class_<Pool>("PoolAllocator")
      .constructor<>()
      .function(
          "allocate",
          +[](Pool& self, size_t size) -> uintptr_t {
            if (size > Pool::chunk_size) {
              return 0;
            }
            return reinterpret_cast<uintptr_t>(self.allocate());
          })
      .function(
          "deallocate",
          +[](Pool& self, uintptr_t ptr) {
            self.deallocate(reinterpret_cast<std::byte*>(ptr));
          })
      .function("reset", &Pool::reset)
      .function("getState", &Pool::get_state);

  emscripten::class_<FreeList>("FreeListAllocator")
      .constructor<>()
      .function(
          "allocate",
          +[](FreeList& self, size_t size) -> uintptr_t {
            return reinterpret_cast<uintptr_t>(self.allocate(size, ALIGNMENT));
          })
      .function(
          "deallocate",
          +[](FreeList& self, uintptr_t ptr) {
            self.deallocate(reinterpret_cast<std::byte*>(ptr));
          })
      .function("reset", &FreeList::reset)
      .function("getState", &FreeList::get_state);

  emscripten::class_<Buddy>("BuddyAllocator")
      .constructor<>()
      .function(
          "allocate",
          +[](Buddy& self, size_t size) -> uintptr_t {
            return reinterpret_cast<uintptr_t>(self.allocate(size));
          })
      .function(
          "deallocate",
          +[](Buddy& self, uintptr_t ptr) {
            self.deallocate(reinterpret_cast<std::byte*>(ptr));
          })
      .function("reset", &Buddy::reset)
      .function("getState", &Buddy::get_state);
}