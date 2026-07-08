#include "free_list_allocator.h"

#include <benchmark/benchmark.h>

#include "benchmark_setup.h"

namespace allocator::perf {
using FreeListHeapFirst = FreeListAllocator<CAPACITY>;
using FreeListHeapBest =
    FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::HEAP>;
using FreeListStackFirst = FreeListAllocator<CAPACITY, FitStrategy::FIRST, BufferType::STACK>;
using FreeListStackBest =
    FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::STACK>;
using FreeListExternalFirst = FreeListAllocator<CAPACITY, FitStrategy::FIRST, BufferType::EXTERNAL>;
using FreeListExternalBest =
    FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::EXTERNAL>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////

BENCHMARK(BM_Allocation<FreeListHeapFirst>)->Name("BM_Allocation/FreeList/Heap/FirstFit");
BENCHMARK(BM_Allocation<FreeListHeapBest>)->Name("BM_Allocation/FreeList/Heap/BestFit");

BENCHMARK(BM_Allocation<FreeListStackFirst>)->Name("BM_Allocation/FreeList/Stack/FirstFit");
BENCHMARK(BM_Allocation<FreeListStackBest>)->Name("BM_Allocation/FreeList/Stack/BestFit");

BENCHMARK(BM_Allocation<FreeListExternalFirst>)->Name("BM_Allocation/FreeList/External/FirstFit");
BENCHMARK(BM_Allocation<FreeListExternalBest>)->Name("BM_Allocation/FreeList/External/BestFit");

//////////////////////////////
// emplace benchmarks
//////////////////////////////

BENCHMARK(BM_Emplace<FreeListHeapFirst>)->Name("BM_Emplace/FreeList/Heap/FirstFit");
BENCHMARK(BM_Emplace<FreeListHeapBest>)->Name("BM_Emplace/FreeList/Heap/BestFit");

BENCHMARK(BM_Emplace<FreeListStackFirst>)->Name("BM_Emplace/FreeList/Stack/FirstFit");
BENCHMARK(BM_Emplace<FreeListStackBest>)->Name("BM_Emplace/FreeList/Stack/BestFit");

BENCHMARK(BM_Emplace<FreeListExternalFirst>)->Name("BM_Emplace/FreeList/External/FirstFit");
BENCHMARK(BM_Emplace<FreeListExternalBest>)->Name("BM_Emplace/FreeList/External/BestFit");


//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<FreeListHeapFirst>)->Name("BM_Workload/FreeList/Heap/FirstFit");
BENCHMARK(BM_Workload<FreeListHeapBest>)->Name("BM_Workload/FreeList/Heap/BestFit");

BENCHMARK(BM_Workload<FreeListStackFirst>)->Name("BM_Workload/FreeList/Stack/FirstFit");
BENCHMARK(BM_Workload<FreeListStackBest>)->Name("BM_Workload/FreeList/Stack/BestFit");

BENCHMARK(BM_Workload<FreeListExternalFirst>)->Name("BM_Workload/FreeList/External/FirstFit");
BENCHMARK(BM_Workload<FreeListExternalBest>)->Name("BM_Workload/FreeList/External/BestFit");

}  // namespace allocator::perf