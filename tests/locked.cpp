#include "locked.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <latch>
#include <ranges>
#include <thread>
#include <vector>

#include "buddy/buddy_allocator.h"
#include "free_list/free_list_allocator.h"
#include "linear/linear_allocator.h"
#include "pool/pool_allocator.h"
#include "test_common.h"

namespace allocator::tests {
template <typename Allocator>
class LockedThreadSafeAllocatorTypedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if constexpr (Allocator::buffer_type == BufferType::EXTERNAL) {
      alloc = std::make_unique<Locked<Allocator>>(buf);
    } else {
      alloc = std::make_unique<Locked<Allocator>>();
    }
  }

  std::byte* allocate_once(size_t bytes = alloc_size,
                           size_t alignment = alloc_alignment) {
    if constexpr (requires(Allocator& a) { a.allocate(); }) {
      return alloc->allocate();  // no arguments, i.e pool
    } else if constexpr (requires(Allocator& a) { a.allocate(bytes); }) {
      return alloc->allocate(bytes);  // size, i.e buddy
    } else {
      return alloc->allocate(
          bytes, alignment);  // size and alignment, i.e linear and free list
    }
  }

  static constexpr size_t compute_size() {
    if constexpr (requires {
                    Allocator::chunk_size;
                    Allocator::chunk_count;
                  }) {
      return Allocator::chunk_size * Allocator::chunk_count;
    } else {
      return Allocator::size;
    }
  }

  size_t measure_capacity() {
    size_t c{};
    while (this->allocate_once()) {
      if (++c > size) {
        ADD_FAILURE() << "allocator never exhausted";
        break;
      }
    }
    alloc->reset();
    return c;
  }

  template <typename F>
  static void run_threads(int k, F&& f) {
    std::latch start{k};
    std::vector<std::thread> threads{};
    threads.reserve(k);

    for (int t{}; t < k; ++t) {
      threads.emplace_back([&, t] {
        start.arrive_and_wait();
        f(t);
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }
  }

  static constexpr bool can_deallocate{
      requires(Allocator & a, std::byte* p){a.deallocate(p);
}
};  // namespace allocator::tests

std::unique_ptr<Locked<Allocator>> alloc{};

// for buffertype::external allocator
static constexpr size_t size{compute_size()};
std::array<std::byte, size> buf{};

static constexpr size_t alloc_size{64};
static constexpr size_t alloc_alignment{8};
}
;

using AllocatorTypes = ::testing::Types<
    LinearAllocator<1024>, PoolAllocator<1024, 128>, FreeListAllocator<1024>,
    BuddyAllocator<1024>, LinearAllocator<1024, BufferType::STACK>,
    PoolAllocator<1024, 128, BufferType::STACK>,
    FreeListAllocator<1024, FitStrategy::FIRST, BufferType::STACK>,
    BuddyAllocator<1024, BufferType::STACK>,
    LinearAllocator<1024, BufferType::EXTERNAL>,
    PoolAllocator<1024, 128, BufferType::EXTERNAL>,
    FreeListAllocator<1024, FitStrategy::FIRST, BufferType::EXTERNAL>,
    BuddyAllocator<1024, BufferType::EXTERNAL>>;

TYPED_TEST_SUITE(LockedThreadSafeAllocatorTypedTest, AllocatorTypes);

TYPED_TEST(LockedThreadSafeAllocatorTypedTest, BasicAllocation) {
  auto* ptr1{this->allocate_once()};
  ASSERT_NE(ptr1, nullptr);

  auto* ptr2{this->allocate_once()};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_NE(ptr1, ptr2);
}

TYPED_TEST(LockedThreadSafeAllocatorTypedTest, ReturnsNullptrWhenOutOfMemory) {
  std::vector<std::byte*> pointers{};

  while (auto* ptr{this->allocate_once()}) {
    pointers.push_back(ptr);
  }

  EXPECT_FALSE(pointers.empty());
  EXPECT_EQ(this->allocate_once(), nullptr);
  EXPECT_GT(this->alloc->get_used(), size_t{0});
}

TYPED_TEST(LockedThreadSafeAllocatorTypedTest, ReleasesMemory) {
  auto* ptr1{this->allocate_once()};
  ASSERT_NE(ptr1, nullptr);

  const size_t used_before{this->alloc->get_used()};

  if constexpr (this->can_deallocate) {
    this->alloc->deallocate(ptr1);
    EXPECT_LT(this->alloc->get_used(), used_before);

    auto* ptr2{this->allocate_once()};
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(ptr1, ptr2);  // should point to the same memory
  } else {
    auto* resized{this->alloc->resize_last(ptr1, this->alloc_size / 2,
                                           this->alloc_alignment)};
    EXPECT_EQ(ptr1, resized);
    EXPECT_LT(this->alloc->get_used(), used_before);
  }
}

TYPED_TEST(LockedThreadSafeAllocatorTypedTest, ResetsSuccessfully) {
  auto* ptr1{this->allocate_once()};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->reset();

  auto* ptr2{this->allocate_once()};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);  // should point to the same memory
}

TYPED_TEST(LockedThreadSafeAllocatorTypedTest,
           Concurrent_NoDuplicateOrOverlap) {
  constexpr int k{16};
  std::vector<std::vector<std::byte*>> thread_results(k);

  this->run_threads(k, [&](int t) {
    while (auto* ptr{this->allocate_once()}) {
      thread_results[t].push_back(ptr);
    }
  });

  std::vector<std::byte*> all_results{};
  std::ranges::copy(thread_results | std::views::join,
                    std::back_inserter(all_results));
  ASSERT_FALSE(all_results.empty());

  std::ranges::sort(all_results, std::less<>{});

  EXPECT_EQ(std::ranges::adjacent_find(all_results), all_results.end())
      << "pointers are duplicated";

  for (size_t i{1}; i < all_results.size(); ++i) {
    EXPECT_GE(static_cast<size_t>(all_results[i] - all_results[i - 1]),
              this->alloc_size)
        << "allocations " << i - 1 << " and " << i << " overlap";
  }

  EXPECT_LE(all_results.size(), this->size);
}

TYPED_TEST(LockedThreadSafeAllocatorTypedTest, Concurrent_Exhaustion) {
  const size_t capacity{this->measure_capacity()};

  constexpr int k{16};
  std::vector<std::vector<std::byte*>> thread_results(k);

  this->run_threads(k, [&](int t) {
    while (auto* ptr{this->allocate_once()}) {
      thread_results[t].push_back(ptr);
    }
  });

  std::vector<std::byte*> all_results{};
  std::ranges::copy(thread_results | std::views::join,
                    std::back_inserter(all_results));
  ASSERT_FALSE(all_results.empty());

  EXPECT_EQ(all_results.size(), capacity);
}

TYPED_TEST(LockedThreadSafeAllocatorTypedTest,
           Concurrent_ReadingVersusWritingMetrics) {
  constexpr int iterations{500};

  constexpr int k_writers{8};
  constexpr int k_readers{8};

  std::atomic<bool> done{};
  std::atomic<int> violations{};
  std::atomic<int> writers_left{k_writers};

  this->run_threads(k_writers + k_readers, [&](int t) {
    if (t < k_writers) {
      for (int i{}; i < iterations; ++i) {
        if (auto* ptr{this->allocate_once()}) {
          std::this_thread::yield();
          if constexpr (this->can_deallocate) {
            this->alloc->deallocate(ptr);
          }
        }
      }

      --writers_left;
      if (writers_left.load() == 0) done.store(true);
    } else {
      while (!done.load()) {
        if (this->alloc->get_used() > this->size) ++violations;
        if (this->alloc->get_free() > this->size) ++violations;
      }
    }
  });

  EXPECT_EQ(violations.load(), 0);
}

TYPED_TEST(LockedThreadSafeAllocatorTypedTest, Concurrent_ResetAfterJoin) {
  const size_t capacity{this->measure_capacity()};

  constexpr int k{16};

  this->run_threads(k, [&](int t) {
    for (int i{}; i < 100; ++i) {
      this->allocate_once();  // use and leak all since reset will reclaim
    }
  });

  EXPECT_GT(this->alloc->get_used(), size_t{0});

  this->alloc->reset();

  EXPECT_EQ(this->alloc->get_used(), size_t{0});
  EXPECT_EQ(this->measure_capacity(), capacity);
}

}  // namespace allocator::tests
