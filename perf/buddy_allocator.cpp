#include "buddy_allocator.h"

#include <benchmark/benchmark.h>

#include "benchmark_setup.h"

namespace allocator::perf {
using BuddyAllocatorHeap = BuddyAllocator<CAPACITY>;
using BuddyAllocatorHeapTracked = BuddyAllocator<CAPACITY, BufferType::HEAP, Tracking::ENABLED>;

using BuddyAllocatorStack = BuddyAllocator<CAPACITY, BufferType::STACK>;
using BuddyAllocatorStackTracked = BuddyAllocator<CAPACITY, BufferType::STACK, Tracking::ENABLED>;

using BuddyAllocatorExternal = BuddyAllocator<CAPACITY, BufferType::EXTERNAL>;
using BuddyAllocatorExternalTracked = BuddyAllocator<CAPACITY, BufferType::EXTERNAL, Tracking::ENABLED>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////

BENCHMARK(BM_Allocation<BuddyAllocatorHeap>)->Name("BM_Allocation/Buddy/Heap");
BENCHMARK(BM_Allocation<BuddyAllocatorHeapTracked>)->Name("BM_Allocation/Buddy/Heap/Tracked");

BENCHMARK(BM_Allocation<BuddyAllocatorStack>)->Name("BM_Allocation/Buddy/Stack");
BENCHMARK(BM_Allocation<BuddyAllocatorStackTracked>)->Name("BM_Allocation/Buddy/Stack/Tracked");

BENCHMARK(BM_Allocation<BuddyAllocatorExternal>)->Name("BM_Allocation/Buddy/External");
BENCHMARK(BM_Allocation<BuddyAllocatorExternalTracked>)->Name("BM_Allocation/Buddy/External/Tracked");

//////////////////////////////
// emplace benchmarks
//////////////////////////////

BENCHMARK(BM_Emplace<BuddyAllocatorHeap>)->Name("BM_Emplace/Buddy/Heap");
BENCHMARK(BM_Emplace<BuddyAllocatorHeapTracked>)->Name("BM_Emplace/Buddy/Heap/Tracked");

BENCHMARK(BM_Emplace<BuddyAllocatorStack>)->Name("BM_Emplace/Buddy/Stack");
BENCHMARK(BM_Emplace<BuddyAllocatorStackTracked>)->Name("BM_Emplace/Buddy/Stack/Tracked");

BENCHMARK(BM_Emplace<BuddyAllocatorExternal>)->Name("BM_Emplace/Buddy/External");
BENCHMARK(BM_Emplace<BuddyAllocatorExternalTracked>)->Name("BM_Emplace/Buddy/External/Tracked");

//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<BuddyAllocatorHeap>)->Name("BM_Workload/Buddy/Heap");
BENCHMARK(BM_Workload<BuddyAllocatorHeapTracked>)->Name("BM_Workload/Buddy/Heap/Tracked");

BENCHMARK(BM_Workload<BuddyAllocatorStack>)->Name("BM_Workload/Buddy/Stack");
BENCHMARK(BM_Workload<BuddyAllocatorStackTracked>)->Name("BM_Workload/Buddy/Stack/Tracked");

BENCHMARK(BM_Workload<BuddyAllocatorExternal>)->Name("BM_Workload/Buddy/External");
BENCHMARK(BM_Workload<BuddyAllocatorExternalTracked>)->Name("BM_Workload/Buddy/External/Tracked");

}  // namespace allocator::perf