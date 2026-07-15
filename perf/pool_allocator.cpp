#include "pool_allocator.h"

#include <benchmark/benchmark.h>

#include "benchmark_setup.h"

namespace allocator::perf {
using PoolAllocatorHeap = PoolAllocator<CAPACITY, CHUNK_SIZE>;
using PoolAllocatorHeapTracked = PoolAllocator<CAPACITY, CHUNK_SIZE, BufferType::HEAP, Tracking::ENABLED>;

using PoolAllocatorStack = PoolAllocator<CAPACITY, CHUNK_SIZE, BufferType::STACK>;
using PoolAllocatorStackTracked = PoolAllocator<CAPACITY, CHUNK_SIZE, BufferType::STACK, Tracking::ENABLED>;

using PoolAllocatorExternal = PoolAllocator<CAPACITY, CHUNK_SIZE, BufferType::EXTERNAL>;
using PoolAllocatorExternalTracked = PoolAllocator<CAPACITY, CHUNK_SIZE, BufferType::EXTERNAL, Tracking::ENABLED>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////

BENCHMARK(BM_Allocation<PoolAllocatorHeap>)->Name("BM_Allocation/Pool/Heap");
BENCHMARK(BM_Allocation<PoolAllocatorHeapTracked>)->Name("BM_Allocation/Pool/Heap/Tracked");

BENCHMARK(BM_Allocation<PoolAllocatorStack>)->Name("BM_Allocation/Pool/Stack");
BENCHMARK(BM_Allocation<PoolAllocatorStackTracked>)->Name("BM_Allocation/Pool/Stack/Tracked");

BENCHMARK(BM_Allocation<PoolAllocatorExternal>)->Name("BM_Allocation/Pool/External");
BENCHMARK(BM_Allocation<PoolAllocatorExternalTracked>)->Name("BM_Allocation/Pool/External/Tracked");

//////////////////////////////
// emplace benchmarks
//////////////////////////////

BENCHMARK(BM_Emplace<PoolAllocatorHeap>)->Name("BM_Emplace/Pool/Heap");
BENCHMARK(BM_Emplace<PoolAllocatorHeapTracked>)->Name("BM_Emplace/Pool/Heap/Tracked");

BENCHMARK(BM_Emplace<PoolAllocatorStack>)->Name("BM_Emplace/Pool/Stack");
BENCHMARK(BM_Emplace<PoolAllocatorStackTracked>)->Name("BM_Emplace/Pool/Stack/Tracked");

BENCHMARK(BM_Emplace<PoolAllocatorExternal>)->Name("BM_Emplace/Pool/External");
BENCHMARK(BM_Emplace<PoolAllocatorExternalTracked>)->Name("BM_Emplace/Pool/External/Tracked");

//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<PoolAllocatorHeap>)->Name("BM_Workload/Pool/Heap");
BENCHMARK(BM_Workload<PoolAllocatorHeapTracked>)->Name("BM_Workload/Pool/Heap/Tracked");

BENCHMARK(BM_Workload<PoolAllocatorStack>)->Name("BM_Workload/Pool/Stack");
BENCHMARK(BM_Workload<PoolAllocatorStackTracked>)->Name("BM_Workload/Pool/Stack/Tracked");

BENCHMARK(BM_Workload<PoolAllocatorExternal>)->Name("BM_Workload/Pool/External");
BENCHMARK(BM_Workload<PoolAllocatorExternalTracked>)->Name("BM_Workload/Pool/External/Tracked");

}  // namespace allocator::perf