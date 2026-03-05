#include "benchmark_setup.h"
#include "linear_allocator.h"

#include <benchmark/benchmark.h>

namespace allocator::perf {

using LinearHeap = LinearAllocator<CAPACITY>;
using LinearStack = LinearAllocator<CAPACITY, BufferType::STACK>;
using LinearExternal = LinearAllocator<CAPACITY, BufferType::EXTERNAL>;

//////////////////////////////
// allocation benchmarks
//////////////////////////////
BENCHMARK(BM_Allocation<LinearHeap>)->Name("BM_Allocation/Linear/Heap");
BENCHMARK(BM_Allocation<LinearStack>)->Name("BM_Allocation/Linear/Stack");
BENCHMARK(BM_Allocation<LinearExternal>)->Name("BM_Allocation/Linear/External");

//////////////////////////////
// emplace benchmarks
//////////////////////////////
BENCHMARK(BM_Emplace<LinearHeap>)->Name("BM_Emplace/Linear/Heap");
BENCHMARK(BM_Emplace<LinearStack>)->Name("BM_Emplace/Linear/Stack");
BENCHMARK(BM_Emplace<LinearExternal>)->Name("BM_Emplace/Linear/External");

//////////////////////////////
// workload benchmarks
//////////////////////////////
BENCHMARK(BM_Workload<LinearHeap>)->Name("BM_Workload/Linear/Heap");
BENCHMARK(BM_Workload<LinearStack>)->Name("BM_Workload/Linear/Stack");
BENCHMARK(BM_Workload<LinearExternal>)->Name("BM_Workload/Linear/External");

}  // namespace allocator::perf