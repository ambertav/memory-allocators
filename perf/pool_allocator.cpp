#include "pool_allocator.h"

#include <benchmark/benchmark.h>

#include "benchmark_setup.h"

namespace allocator::perf {
using PoolAllocatorHeap = PoolAllocator<CAPACITY, CHUNK_SIZE>;
using PoolAllocatorStack =
    PoolAllocator<CAPACITY, CHUNK_SIZE, BufferType::STACK>;
using PoolAllocatorExternal =
    PoolAllocator<CAPACITY, CHUNK_SIZE, BufferType::EXTERNAL>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////

BENCHMARK(BM_Allocation<PoolAllocatorHeap>)->Name("BM_Allocation/Pool/Heap");
BENCHMARK(BM_Allocation<PoolAllocatorStack>)->Name("BM_Allocation/Pool/Stack");
BENCHMARK(BM_Allocation<PoolAllocatorExternal>)
    ->Name("BM_Allocation/Pool/External");

//////////////////////////////
// emplace benchmarks
//////////////////////////////

BENCHMARK(BM_Emplace<PoolAllocatorHeap>)->Name("BM_Emplace/Pool/Heap");
BENCHMARK(BM_Emplace<PoolAllocatorStack>)->Name("BM_Emplace/Pool/Stack");
BENCHMARK(BM_Emplace<PoolAllocatorExternal>)->Name("BM_Emplace/Pool/External");

//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<PoolAllocatorHeap>)->Name("BM_Workload/Pool/Heap");
BENCHMARK(BM_Workload<PoolAllocatorStack>)->Name("BM_Workload/Pool/Stack");
BENCHMARK(BM_Workload<PoolAllocatorExternal>)
    ->Name("BM_Workload/Pool/External");

}  // namespace allocator::perf