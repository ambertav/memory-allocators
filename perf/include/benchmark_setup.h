#pragma once

#include <benchmark/benchmark.h>

#include <memory>
#include <span>

#include "common.h"

namespace allocator::perf {
inline constexpr size_t CAPACITY{65536};
inline constexpr int ROUNDS{100};

struct Obj {
  int x;
  double y;
  Obj(int a, double b) : x(a), y(b) {}
};

template <typename Allocator>
struct Setup {
  std::unique_ptr<Allocator> alloc{};
  std::array<std::byte, CAPACITY> buf{};

  Setup() {
    if constexpr (Allocator::buffer_type == BufferType::EXTERNAL) {
      alloc = std::make_unique<Allocator>(buf);
    } else {
      alloc = std::make_unique<Allocator>();
    }
  }
};

template <typename Allocator>
inline void BM_Allocation(::benchmark::State& state) {
  Setup<Allocator> setup{};
  for (auto _ : state) {
    std::byte* ptr{setup.alloc->allocate(64, 8)};
    ::benchmark::DoNotOptimize(ptr);
  }
  state.SetItemsProcessed(state.iterations());
}

template <typename Allocator>
inline void BM_Emplace(::benchmark::State& state) {
  Setup<Allocator> setup{};
  for (auto _ : state) {
    Obj* obj{setup.alloc->template emplace<Obj>(15, 3.14)};
    ::benchmark::DoNotOptimize(obj);
    setup.alloc->template destroy<Obj>(obj);
  }
  state.SetItemsProcessed(state.iterations());
}

//////////////////////////////
// workload benchmarks
//////////////////////////////

template <typename Allocator>
inline void BM_Workload(::benchmark::State& state) {
  Setup<Allocator> setup{};
  Obj* objects[ROUNDS];

  for (auto _ : state) {
    for (int i{}; i < ROUNDS; ++i) {
      objects[i] = setup.alloc->template emplace<Obj>(i, i * 1.5);
      ::benchmark::DoNotOptimize(objects[i]);
    }

    for (int i{}; i < ROUNDS; ++i) {
      setup.alloc->template destroy<Obj>(objects[i]);
    }

    setup.alloc->reset();
  }
  state.SetItemsProcessed(state.iterations() * ROUNDS);
}

}  // namespace allocator::perf