#include "benchmark_setup.h"
#include "linear_allocator.h"

#include <benchmark/benchmark.h>

namespace allocator::perf {

using LinearHeap = LinearAllocator<CAPACITY>;
using LinearHeapTracked = LinearAllocator<CAPACITY, BufferType::HEAP, Tracking::ENABLED>;

using LinearStack = LinearAllocator<CAPACITY, BufferType::STACK>;
using LinearStackTracked = LinearAllocator<CAPACITY, BufferType::STACK, Tracking::ENABLED>;

using LinearExternal = LinearAllocator<CAPACITY, BufferType::EXTERNAL>;
using LinearExternalTracked = LinearAllocator<CAPACITY, BufferType::EXTERNAL, Tracking::ENABLED>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////

BENCHMARK(BM_Allocation<LinearHeap>)->Name("BM_Allocation/Linear/Heap");
BENCHMARK(BM_Allocation<LinearHeapTracked>)->Name("BM_Allocation/Linear/Heap/Tracked");

BENCHMARK(BM_Allocation<LinearStack>)->Name("BM_Allocation/Linear/Stack");
BENCHMARK(BM_Allocation<LinearStackTracked>)->Name("BM_Allocation/Linear/Stack/Tracked");

BENCHMARK(BM_Allocation<LinearExternal>)->Name("BM_Allocation/Linear/External");
BENCHMARK(BM_Allocation<LinearExternalTracked>)->Name("BM_Allocation/Linear/External/Tracked");

//////////////////////////////
// emplace benchmarks
//////////////////////////////

BENCHMARK(BM_Emplace<LinearHeap>)->Name("BM_Emplace/Linear/Heap");
BENCHMARK(BM_Emplace<LinearHeapTracked>)->Name("BM_Emplace/Linear/Heap/Tracked");

BENCHMARK(BM_Emplace<LinearStack>)->Name("BM_Emplace/Linear/Stack");
BENCHMARK(BM_Emplace<LinearStackTracked>)->Name("BM_Emplace/Linear/Stack/Tracked");

BENCHMARK(BM_Emplace<LinearExternal>)->Name("BM_Emplace/Linear/External");
BENCHMARK(BM_Emplace<LinearExternalTracked>)->Name("BM_Emplace/Linear/External/Tracked");

//////////////////////////////
// workload benchmarks
//////////////////////////////

BENCHMARK(BM_Workload<LinearHeap>)->Name("BM_Workload/Linear/Heap");
BENCHMARK(BM_Workload<LinearHeapTracked>)->Name("BM_Workload/Linear/Heap/Tracked");

BENCHMARK(BM_Workload<LinearStack>)->Name("BM_Workload/Linear/Stack");
BENCHMARK(BM_Workload<LinearStackTracked>)->Name("BM_Workload/Linear/Stack/Tracked");

BENCHMARK(BM_Workload<LinearExternal>)->Name("BM_Workload/Linear/External");
BENCHMARK(BM_Workload<LinearExternalTracked>)->Name("BM_Workload/Linear/External/Tracked");

}  // namespace allocator::perf