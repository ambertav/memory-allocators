#pragma once

namespace allocator {
namespace tests {
struct Obj {
  int x;
  double y;
  Obj(int a, double b) : x(a), y(b) {}
};

struct TrackedObj {
  static inline int destructor_calls = 0;
  int value;

  TrackedObj(int v) : value(v) {}
  ~TrackedObj() { ++destructor_calls; }
};
}  // namespace tests
}  // namespace allocator