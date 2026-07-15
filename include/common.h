#pragma once

#include <cstddef>

namespace allocator {

enum class BufferType { HEAP, STACK, EXTERNAL };
enum class FitStrategy { FIRST, BEST };
enum class Tracking { DISABLED, ENABLED };

inline bool is_valid_alignment(size_t alignment) {
  return alignment > 0 && (alignment & (alignment - 1)) == 0;
}

inline size_t align_forward(size_t offset, size_t alignment) {
  size_t remainder{offset % alignment};
  if (remainder == 0) {
    return offset;
  } else {
    return offset + (alignment - remainder);
  }
}
}  // namespace allocator