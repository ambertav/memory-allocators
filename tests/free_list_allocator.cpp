#include "free_list_allocator.h"

#include <gtest/gtest.h>

#include <memory>
#include <span>

/*
Tests that carry over almost unchanged: BasicAllocation, AlignsCorrectly,
AlignmentsPadsToCreateGaps, ReturnsNullptrWhenOutOfMemory,
InvalidAlignmentReturnsNullptr, TypedAllocateSucceeds,
EmplaceAllocatesAndCreatesInPlace, DestroyCallsDestructor.
*/

namespace allocator::tests {
template <typename Allocator>
class FreeListAllocatorTypedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if constexpr (std::is_same_v<Allocator,
                                 FreeListAllocator<1024, BufferType::EXTERNAL,
                                                   FitStrategy::FIRST>>) {
      buf = std::make_unique<std::byte[]>(buf_size);
      buf_span = std::span(buf.get(), buf_size);
      alloc = std::make_unique<Allocator>(buf_span);
    } else {
      alloc = std::make_unique<Allocator>();
    }
  }

  std::unique_ptr<Allocator> alloc{};

  static constexpr size_t buf_size{1024};
  std::unique_ptr<std::byte[]> buf{};
  std::span<std::byte> buf_span{};
};

// defaults to FitStrategy::FIRST if not specified
using AllocatorTypes = ::testing::Types<
    FreeListAllocator<1024>,
    FreeListAllocator<1024, BufferType::HEAP, FitStrategy::BEST>,
    FreeListAllocator<1024, BufferType::STACK>,
    FreeListAllocator<1024, BufferType::EXTERNAL>>;

TYPED_TEST_SUITE(FreeListAllocatorTypedTest, AllocatorTypes);

TYPED_TEST(FreeListAllocatorTypedTest, BasicAllocation) {
  auto* ptr1{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr1, nullptr);

  auto* ptr2{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_NE(ptr1, ptr2);
}

TYPED_TEST(FreeListAllocatorTypedTest, AlignsCorrectly) {
  auto* ptr1{this->alloc->allocate(13, 1)};
  auto* ptr2{this->alloc->allocate(50, 8)};
  auto* ptr3{this->alloc->allocate(100, 16)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 1, 0);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 8, 0);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr3) % 16, 0);
}

TYPED_TEST(FreeListAllocatorTypedTest, ReturnsNullptrWhenOutOfMemory) {
  auto* ptr{this->alloc->allocate(2000, 8)};
  EXPECT_EQ(ptr, nullptr);
}

TYPED_TEST(FreeListAllocatorTypedTest, DeallocateAndReallocate) {
  size_t size{100};
  auto* ptr1{this->alloc->allocate(size, 8)};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->deallocate(ptr1);

  auto* ptr2{this->alloc->allocate(size, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);  // should point to the same memory
}

TYPED_TEST(FreeListAllocatorTypedTest, DeallocateOutOfOrderReuse) {
  auto* ptr1{this->alloc->allocate(100, 8)};
  auto* ptr2{this->alloc->allocate(100, 8)};
  auto* ptr3{this->alloc->allocate(100, 8)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  // deallocate ptr1 and ptr2
  this->alloc->deallocate(ptr2);
  this->alloc->deallocate(ptr1);

  // reallocate ptr4 and ptr5 in their place
  auto* ptr4{this->alloc->allocate(100, 8)};
  auto* ptr5{this->alloc->allocate(100, 8)};

  ASSERT_NE(ptr4, nullptr);
  ASSERT_NE(ptr5, nullptr);

  EXPECT_NE(ptr4, ptr5);

  EXPECT_TRUE(ptr1 == ptr4 || ptr1 == ptr5);
  EXPECT_TRUE(ptr2 == ptr4 || ptr2 == ptr5);
}

TYPED_TEST(FreeListAllocatorTypedTest, DeallocateNullptr) {
  auto* ptr1{this->alloc->allocate(100, 8)};
  ASSERT_NE(ptr1, nullptr);

  size_t used_before_dealloc{this->alloc->get_used()};
  size_t free_before_dealloc{this->alloc->get_free()};

  this->alloc->deallocate(nullptr);

  size_t used_after_dealloc{this->alloc->get_used()};
  size_t free_after_dealloc{this->alloc->get_free()};

  // allocator state should not have changed
  EXPECT_EQ(used_before_dealloc, used_after_dealloc);
  EXPECT_EQ(free_before_dealloc, free_after_dealloc);

  auto* ptr2{this->alloc->allocate(200, 8)};
  ASSERT_NE(ptr2, nullptr);
  EXPECT_NE(ptr1, ptr2);  // ptr1 is still valid
}

TYPED_TEST(FreeListAllocatorTypedTest, DeallocateOutOfBoundsPointer) {
  auto* valid{this->alloc->allocate(100, 8)};
  ASSERT_NE(valid, nullptr);

  std::byte* invalid{valid + 10000};
  EXPECT_DEATH(this->alloc->deallocate(invalid), "pointer is out of bounds");
}

TYPED_TEST(FreeListAllocatorTypedTest, FragmentationAndCoalescing) {
  auto* ptr1{this->alloc->allocate(300, 8)};
  auto* ptr2{this->alloc->allocate(300, 8)};
  auto* ptr3{this->alloc->allocate(300, 8)};

  ASSERT_NE(ptr1, nullptr);
  ASSERT_NE(ptr2, nullptr);
  ASSERT_NE(ptr3, nullptr);

  this->alloc->deallocate(ptr1);
  this->alloc->deallocate(ptr2);
  this->alloc->deallocate(ptr3);

  auto* large{this->alloc->allocate(850, 8)};
  EXPECT_NE(large, nullptr);
}

TYPED_TEST(FreeListAllocatorTypedTest, ResetsSuccessfully) {
  auto* ptr1{this->alloc->allocate(500, 8)};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->reset();

  auto* ptr2{this->alloc->allocate(500, 8)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);  // should point to the same memory
}

TYPED_TEST(FreeListAllocatorTypedTest, InvalidAligmentReturnsNullptr) {
  EXPECT_EQ(this->alloc->allocate(100, 0), nullptr);
  EXPECT_EQ(this->alloc->allocate(100, 3), nullptr);
  EXPECT_EQ(this->alloc->allocate(100, 6), nullptr);
}

TYPED_TEST(FreeListAllocatorTypedTest, TypedAllocateSucceeds) {
  int n{10};
  int* ptr{this->alloc->template allocate<int>(n)};
  ASSERT_NE(ptr, nullptr);

  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) & alignof(int), 0);

  for (int i{}; i < n; ++i) {
    ptr[i] = i;
    EXPECT_EQ(ptr[i], i);
  }
}

TYPED_TEST(FreeListAllocatorTypedTest, TypedDeallocateSucceeds) {
  int n{10};
  int* ptr1{this->alloc->template allocate<int>(n)};
  ASSERT_NE(ptr1, nullptr);

  this->alloc->template deallocate<int>(ptr1);

  int* ptr2{this->alloc->template allocate<int>(n)};
  ASSERT_NE(ptr2, nullptr);

  EXPECT_EQ(ptr1, ptr2);
}

TYPED_TEST(FreeListAllocatorTypedTest, EmplaceAllocatesAndCreatesInPlace) {
  int a{15};
  double b{3.14};
  Obj* obj{this->alloc->template emplace<Obj>(a, b)};
  ASSERT_NE(obj, nullptr);

  // check construction
  EXPECT_EQ(obj->x, a);
  EXPECT_EQ(obj->y, b);

  this->alloc->template destroy<Obj>(obj);
}

TYPED_TEST(FreeListAllocatorTypedTest, DestroyCallsDestructor) {
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