#include "pool_allocator.h"
#include "test_common.h"

#include <gtest/gtest.h>

#include <memory>

namespace allocator::tests {
template <typename Allocator>
class PoolAllocatorTypedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if constexpr (Allocator::buffer_type == BufferType::EXTERNAL) {
      alloc = std::make_unique<Allocator>(buf);
    } else {
      alloc = std::make_unique<Allocator>();
    }
  }

  std::unique_ptr<Allocator> alloc{};

  static constexpr size_t chunk_size{Allocator::chunk_size};
  static constexpr size_t chunk_count{Allocator::chunk_count};

  // for buffertype::external allocator
  static constexpr size_t buf_size{1024};
  std::array<std::byte, buf_size> buf{};
};

using AllocatorTypes = ::testing::Types<
    PoolAllocator<1024, 128>,                         // heap
    PoolAllocator<1024, 128, BufferType::STACK>,      // stack
    PoolAllocator<1024, 128, BufferType::EXTERNAL>>;  // external

TYPED_TEST_SUITE(PoolAllocatorTypedTest, AllocatorTypes);

TYPED_TEST(PoolAllocatorTypedTest, BasicAllocation) {
  auto* ptr1{this->alloc->allocate()};
  ASSERT_NE(ptr1, nullptr);

  auto* ptr2{this->alloc->allocate()};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_NE(ptr1, ptr2);
  EXPECT_EQ(ptr2 - ptr1, static_cast<ptrdiff_t>(this->chunk_size));
}

TYPED_TEST(PoolAllocatorTypedTest, ReturnsNullptrWhenOutOfMemory) {
  std::vector<std::byte*> pointers{this->chunk_count, nullptr};

  for (size_t i{}; i < this->chunk_count; ++i) {
    pointers[i] = this->alloc->allocate();
    ASSERT_NE(pointers[i], nullptr);
  }

  EXPECT_EQ(this->alloc->get_used(), 1024);

  auto* ptr{this->alloc->allocate()};
  EXPECT_EQ(ptr, nullptr);
}

TYPED_TEST(PoolAllocatorTypedTest, DeallocateAndReallocate) {
  auto* ptr1{this->alloc->allocate()};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->deallocate(ptr1);

  auto* ptr2{this->alloc->allocate()};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);  // should point to the same memory
}

TYPED_TEST(PoolAllocatorTypedTest, DeallocateOutOfOrderReuse) {
  auto* ptr1{this->alloc->allocate()};
  auto* ptr2{this->alloc->allocate()};
  auto* ptr3{this->alloc->allocate()};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  // deallocate ptr1 and ptr2
  this->alloc->deallocate(ptr1);
  this->alloc->deallocate(ptr2);

  // reallocate ptr4 and ptr5 in their place
  auto* ptr4{this->alloc->allocate()};
  auto* ptr5{this->alloc->allocate()};

  ASSERT_NE(ptr4, nullptr);
  ASSERT_NE(ptr5, nullptr);

  EXPECT_TRUE(ptr1 == ptr5);
  EXPECT_TRUE(ptr2 == ptr4);
}

TYPED_TEST(PoolAllocatorTypedTest, DeallocateNullptr) {
  auto* ptr1{this->alloc->allocate()};
  ASSERT_NE(ptr1, nullptr);

  size_t used_before_dealloc{this->alloc->get_used()};
  size_t free_before_dealloc{this->alloc->get_free()};

  this->alloc->deallocate(nullptr);

  size_t used_after_dealloc{this->alloc->get_used()};
  size_t free_after_dealloc{this->alloc->get_free()};

  // allocator state should not have changed
  EXPECT_EQ(used_before_dealloc, used_after_dealloc);
  EXPECT_EQ(free_before_dealloc, free_after_dealloc);

  auto* ptr2{this->alloc->allocate()};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_NE(ptr1, ptr2);
}

TYPED_TEST(PoolAllocatorTypedTest, DeallocateOutOfBoundsPointer) {
  auto* valid{this->alloc->allocate()};
  ASSERT_NE(valid, nullptr);

  std::byte* invalid{valid + 100000};
  EXPECT_DEATH(this->alloc->deallocate(invalid), "pointer is out of bounds");
}

TYPED_TEST(PoolAllocatorTypedTest, ResetsSuccessfully) {
  auto* ptr1{this->alloc->allocate()};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->reset();

  auto* ptr2{this->alloc->allocate()};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);
}

TYPED_TEST(PoolAllocatorTypedTest, TypedAllocateSucceeds) {
  int* ptr{this->alloc->template allocate<int>()};
  ASSERT_NE(ptr, nullptr);

  *ptr = 42;
  EXPECT_EQ(*ptr, 42);
}

TYPED_TEST(PoolAllocatorTypedTest, TypedDeallocateSucceeds) {
  int* ptr1{this->alloc->template allocate<int>()};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->template deallocate<int>(ptr1);

  int* ptr2{this->alloc->template allocate<int>()};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);
}

TYPED_TEST(PoolAllocatorTypedTest, EmplaceAllocatesAndCreatesInPlace) {
  int a{15};
  double b{3.14};

  Obj* obj{this->alloc->template emplace<Obj>(a, b)};
  ASSERT_NE(obj, nullptr);

  // check construciton
  EXPECT_EQ(obj->x, a);
  EXPECT_EQ(obj->y, b);

  this->alloc->template destroy<Obj>(obj);
}

TYPED_TEST(PoolAllocatorTypedTest, DestroyCallsDestructor) {
  TrackedObj::destructor_calls = 0;

  TrackedObj* obj1{this->alloc->template emplace<TrackedObj>(10)};
  TrackedObj* obj2{this->alloc->template emplace<TrackedObj>(10)};
  TrackedObj* obj3{this->alloc->template emplace<TrackedObj>(10)};

  ASSERT_NE(obj1, nullptr);
  ASSERT_NE(obj2, nullptr);
  ASSERT_NE(obj3, nullptr);

  this->alloc->template destroy<TrackedObj>(obj1);
  this->alloc->template destroy<TrackedObj>(obj2);
  this->alloc->template destroy<TrackedObj>(obj3);

  EXPECT_EQ(TrackedObj::destructor_calls, 3);
}

}  // namespace allocator::tests