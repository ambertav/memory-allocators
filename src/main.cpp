#include <iostream>

#include "linear_allocator.h"
#include "free_list_allocator.h"
#include "buddy_allocator.h"
#include "pool_allocator.h"

int main() {
  std::cout << "custom memory allocators\n";

  using namespace allocator;
  FreeListAllocator<1024> alloc{};

  std::cout << alloc.get_state() << "\n";

//   int* ptr{alloc.allocate<int>(10)};
//   for (int i{}; i < 10; ++i) {
//     ptr[i] = i;
//     std::cout << ptr[i] << "\n";
//   }
  return 0;
}