#include "free_list_allocator.h"

#include <benchmark/benchmark.h>

#include "benchmark_setup.h"

namespace allocator::perf {
using FreeListHeapFirst = FreeListAllocator<CAPACITY>;
using FreeListHeapFirstTracked = FreeListAllocator<CAPACITY, FitStrategy::FIRST, BufferType::HEAP, Tracking::ENABLED>;

using FreeListHeapBest = FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::HEAP>;
using FreeListHeapBestTracked = FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::HEAP, Tracking::ENABLED>;

using FreeListStackFirst = FreeListAllocator<CAPACITY, FitStrategy::FIRST, BufferType::STACK>;
using FreeListStackFirstTracked = FreeListAllocator<CAPACITY, FitStrategy::FIRST, BufferType::STACK, Tracking::ENABLED>;

using FreeListStackBest = FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::STACK>;
using FreeListStackBestTracked = FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::STACK, Tracking::ENABLED>;

using FreeListExternalFirst = FreeListAllocator<CAPACITY, FitStrategy::FIRST, BufferType::EXTERNAL>;
using FreeListExternalFirstTracked = FreeListAllocator<CAPACITY, FitStrategy::FIRST, BufferType::EXTERNAL, Tracking::ENABLED>;

using FreeListExternalBest = FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::EXTERNAL>;
using FreeListExternalBestTracked = FreeListAllocator<CAPACITY, FitStrategy::BEST, BufferType::EXTERNAL, Tracking::ENABLED>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////

BENCHMARK(BM_Allocation<FreeListHeapFirst>)->Name("BM_Allocation/FreeList/Heap/FirstFit");
BENCHMARK(BM_Allocation<FreeListHeapFirstTracked>)->Name("BM_Allocation/FreeList/Heap/FirstFit/Tracked");
BENCHMARK(BM_Allocation<FreeListHeapBest>)->Name("BM_Allocation/FreeList/Heap/BestFit");
BENCHMARK(BM_Allocation<FreeListHeapBestTracked>)->Name("BM_Allocation/FreeList/Heap/BestFit/Tracked");

BENCHMARK(BM_Allocation<FreeListStackFirst>)->Name("BM_Allocation/FreeList/Stack/FirstFit");
BENCHMARK(BM_Allocation<FreeListStackFirstTracked>)->Name("BM_Allocation/FreeList/Stack/FirstFit/Tracked");
BENCHMARK(BM_Allocation<FreeListStackBest>)->Name("BM_Allocation/FreeList/Stack/BestFit");
BENCHMARK(BM_Allocation<FreeListStackBestTracked>)->Name("BM_Allocation/FreeList/Stack/BestFit/Tracked");

BENCHMARK(BM_Allocation<FreeListExternalFirst>)->Name("BM_Allocation/FreeList/External/FirstFit");
BENCHMARK(BM_Allocation<FreeListExternalFirstTracked>)->Name("BM_Allocation/FreeList/External/FirstFit/Tracked");
BENCHMARK(BM_Allocation<FreeListExternalBest>)->Name("BM_Allocation/FreeList/External/BestFit");
BENCHMARK(BM_Allocation<FreeListExternalBestTracked>)->Name("BM_Allocation/FreeList/External/BestFit/Tracked");

//////////////////////////////
// emplace benchmarks
//////////////////////////////

BENCHMARK(BM_Emplace<FreeListHeapFirst>)->Name("BM_Emplace/FreeList/Heap/FirstFit");
BENCHMARK(BM_Emplace<FreeListHeapFirstTracked>)->Name("BM_Emplace/FreeList/Heap/FirstFit/Tracked");
BENCHMARK(BM_Emplace<FreeListHeapBest>)->Name("BM_Emplace/FreeList/Heap/BestFit");
BENCHMARK(BM_Emplace<FreeListHeapBestTracked>)->Name("BM_Emplace/FreeList/Heap/BestFit/Tracked");

BENCHMARK(BM_Emplace<FreeListStackFirst>)->Name("BM_Emplace/FreeList/Stack/FirstFit");
BENCHMARK(BM_Emplace<FreeListStackFirstTracked>)->Name("BM_Emplace/FreeList/Stack/FirstFit/Tracked");
BENCHMARK(BM_Emplace<FreeListStackBestTracked>)->Name("BM_Emplace/FreeList/Stack/BestFit");
BENCHMARK(BM_Emplace<FreeListStackBest>)->Name("BM_Emplace/FreeList/Stack/BestFit/Tracked");

BENCHMARK(BM_Emplace<FreeListExternalFirst>)->Name("BM_Emplace/FreeList/External/FirstFit");
BENCHMARK(BM_Emplace<FreeListExternalFirstTracked>)->Name("BM_Emplace/FreeList/External/FirstFit/Tracked");
BENCHMARK(BM_Emplace<FreeListExternalBest>)->Name("BM_Emplace/FreeList/External/BestFit");
BENCHMARK(BM_Emplace<FreeListExternalBestTracked>)->Name("BM_Emplace/FreeList/External/BestFit/Tracked");


//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<FreeListHeapFirst>)->Name("BM_Workload/FreeList/Heap/FirstFit");
BENCHMARK(BM_Workload<FreeListHeapFirstTracked>)->Name("BM_Workload/FreeList/Heap/FirstFit/Tracked");
BENCHMARK(BM_Workload<FreeListHeapBest>)->Name("BM_Workload/FreeList/Heap/BestFit");
BENCHMARK(BM_Workload<FreeListHeapBestTracked>)->Name("BM_Workload/FreeList/Heap/BestFit/Tracked");

BENCHMARK(BM_Workload<FreeListStackFirst>)->Name("BM_Workload/FreeList/Stack/FirstFit");
BENCHMARK(BM_Workload<FreeListStackFirstTracked>)->Name("BM_Workload/FreeList/Stack/FirstFit/Tracked");
BENCHMARK(BM_Workload<FreeListStackBest>)->Name("BM_Workload/FreeList/Stack/BestFit");
BENCHMARK(BM_Workload<FreeListStackBestTracked>)->Name("BM_Workload/FreeList/Stack/BestFit/Tracked");

BENCHMARK(BM_Workload<FreeListExternalFirst>)->Name("BM_Workload/FreeList/External/FirstFit");
BENCHMARK(BM_Workload<FreeListExternalFirstTracked>)->Name("BM_Workload/FreeList/External/FirstFit/Tracked");
BENCHMARK(BM_Workload<FreeListExternalBest>)->Name("BM_Workload/FreeList/External/BestFit");
BENCHMARK(BM_Workload<FreeListExternalBestTracked>)->Name("BM_Workload/FreeList/External/BestFit/Tracked");

}  // namespace allocator::perf