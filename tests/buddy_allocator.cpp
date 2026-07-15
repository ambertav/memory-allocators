#include "buddy_allocator.h"
#include "test_common.h"

#include <gtest/gtest.h>

namespace allocator::tests {
template <typename Allocator>
class BuddyAllocatorTypedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if constexpr (std::is_same_v<Allocator,
                                 BuddyAllocator<1024, BufferType::EXTERNAL>>) {
      alloc = std::make_unique<Allocator>(buf);
    } else {
      alloc = std::make_unique<Allocator>();
    }
  }

  std::unique_ptr<Allocator> alloc{};

  // for buffertype::external allocator
  static constexpr size_t buf_size{1024};
  std::array<std::byte, buf_size> buf{};
};

using AllocatorTypes =
    ::testing::Types<BuddyAllocator<1024>,
                     BuddyAllocator<1024, BufferType::STACK>,
                     BuddyAllocator<1024, BufferType::EXTERNAL>>;

TYPED_TEST_SUITE(BuddyAllocatorTypedTest, AllocatorTypes);

TYPED_TEST(BuddyAllocatorTypedTest, BasicAllocation) {
  auto* ptr1{this->alloc->allocate(100)};
  ASSERT_NE(ptr1, nullptr);

  auto* ptr2{this->alloc->allocate(100)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_NE(ptr1, ptr2);
}

TYPED_TEST(BuddyAllocatorTypedTest, AlignsToPowerOfTwo) {
  auto* ptr{this->alloc->allocate(100)};

  ASSERT_NE(ptr, nullptr);

  size_t used{this->alloc->get_used()};
  EXPECT_EQ(used & (used - 1), 0);
}

TYPED_TEST(BuddyAllocatorTypedTest, ReturnsNullptrWhenOutOfMemory) {
  auto* ptr{this->alloc->allocate(this->buf_size + 1)};
  EXPECT_EQ(ptr, nullptr);
}

TYPED_TEST(BuddyAllocatorTypedTest, DeallocateNullptr) {
  auto* ptr1{this->alloc->allocate(100)};
  ASSERT_NE(ptr1, nullptr);

  size_t used_before_dealloc{this->alloc->get_used()};
  size_t free_before_dealloc{this->alloc->get_free()};

  this->alloc->deallocate(nullptr);

  size_t used_after_dealloc{this->alloc->get_used()};
  size_t free_after_dealloc{this->alloc->get_free()};

  // allocator state should not have changed
  EXPECT_EQ(used_before_dealloc, used_after_dealloc);
  EXPECT_EQ(free_before_dealloc, free_after_dealloc);

  auto* ptr2{this->alloc->allocate(200)};
  ASSERT_NE(ptr2, nullptr);
  EXPECT_NE(ptr1, ptr2);  // ptr1 is still valid
}

TYPED_TEST(BuddyAllocatorTypedTest, DeallocateOutOfBoundsPointer) {
  auto* valid{this->alloc->allocate(100)};
  ASSERT_NE(valid, nullptr);

  std::byte* invalid{valid + 10000};
  EXPECT_DEATH(this->alloc->deallocate(invalid), "pointer is out of bounds");
}

TYPED_TEST(BuddyAllocatorTypedTest, FullCoalescing) {
  size_t alloc_size{this->buf_size / 4};

  auto* ptr1{this->alloc->allocate(alloc_size)};
  auto* ptr2{this->alloc->allocate(alloc_size)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);

  this->alloc->deallocate(ptr1);
  this->alloc->deallocate(ptr2);

  EXPECT_EQ(this->alloc->get_used(), 0);

  auto* large{this->alloc->allocate(alloc_size * 2)};
  EXPECT_NE(large, nullptr);
}

TYPED_TEST(BuddyAllocatorTypedTest, PartialCoalescing) {
  size_t alloc_size{this->buf_size / 4};

  auto* ptr1{this->alloc->allocate(alloc_size)};
  auto* ptr2{this->alloc->allocate(alloc_size)};
  auto* ptr3{this->alloc->allocate(alloc_size)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  this->alloc->deallocate(ptr1);
  this->alloc->deallocate(ptr3);

  EXPECT_GT(this->alloc->get_used(), 0);

  auto* large{this->alloc->allocate(alloc_size * 3)};
  EXPECT_EQ(large, nullptr);

  this->alloc->deallocate(ptr2);
  EXPECT_EQ(this->alloc->get_used(), 0);
}

TYPED_TEST(BuddyAllocatorTypedTest, ResetsSuccessfully) {
  auto* ptr1{this->alloc->allocate(500)};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->reset();

  auto* ptr2{this->alloc->allocate(500)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);  // should point to the same memory
}

TYPED_TEST(BuddyAllocatorTypedTest, TypedAllocateSucceeds) {
  int n{10};
  int* ptr{this->alloc->template allocate<int>(n)};
  ASSERT_NE(ptr, nullptr);

  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(int), 0);

  for (int i{}; i < n; ++i) {
    ptr[i] = i;
    EXPECT_EQ(ptr[i], i);
  }
}

TYPED_TEST(BuddyAllocatorTypedTest, TypedDeallocateSucceeds) {
  int n{10};
  int* ptr1{this->alloc->template allocate<int>(n)};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->template deallocate<int>(ptr1);

  int* ptr2{this->alloc->template allocate<int>(n)};
  ASSERT_NE(ptr2, nullptr);
}

TYPED_TEST(BuddyAllocatorTypedTest, EmplaceAllocatesAndCreatesInPlace) {
  int a{15};
  double b{3.14};
  Obj* obj{this->alloc->template emplace<Obj>(a, b)};
  ASSERT_NE(obj, nullptr);

  // check construction
  EXPECT_EQ(obj->x, a);
  EXPECT_EQ(obj->y, b);

  this->alloc->template destroy<Obj>(obj);
}

TYPED_TEST(BuddyAllocatorTypedTest, DestroyCallsDestructor) {
  TrackedObj::destructor_calls = 0;  // assign to 0 at start of each typed test

  TrackedObj* obj1{this->alloc->template emplace<TrackedObj>(10)};
  TrackedObj* obj2{this->alloc->template emplace<TrackedObj>(10)};
  TrackedObj* obj3{this->alloc->template emplace<TrackedObj>(10)};

  ASSERT_NE(obj1, nullptr);
  ASSERT_NE(obj2, nullptr);
  ASSERT_NE(obj3, nullptr);

  this->alloc->template destroy(obj1);
  this->alloc->template destroy(obj2);
  this->alloc->template destroy(obj3);
  EXPECT_EQ(TrackedObj::destructor_calls, 3);
}

}  // namespace allocator::tests