#include "ext/Configure.h"

#include <gtest/gtest.h>
#include <boost/timer/timer.hpp>

using namespace xi;

template <size_t N, size_t DefaultChunkSize = 64>
struct Chunk {
  static constexpr uint8_t * kEof = (uint8_t*)1;
  static constexpr size_t kBlockSize = alignof (void*);
  static constexpr size_t kElementSize = (N + (kBlockSize - 1))/kBlockSize;

  atomic<uint8_t *> firstFree { kEof };
  atomic<uint8_t *> firstUnallocated;

  uint8_t data [DefaultChunkSize * kBlockSize * kElementSize];
  uint8_t tail [0];

  Chunk() {
    std::cout << "kBlockSize: " << kBlockSize << std::endl;
    std::cout << "kElementSize: " << kElementSize << std::endl;
    firstUnallocated.store (data, ::std::memory_order_relaxed);
  }

  void * allocate() {
    auto * free = firstFree.load (::std::memory_order_relaxed);
    // std::cout << "free:" << (size_t)free << std::endl;
    auto * nextFree = kEof;

    do {
      if (kEof == free) {
        break;
      }
      nextFree = * (uint8_t**)free;
    } while (! firstFree.compare_exchange_weak(free, nextFree,
                                               ::std::memory_order_release,
                                               ::std::memory_order_relaxed
                                               )
             );

    if (kEof == free) {
      free = firstUnallocated.load (::std::memory_order_relaxed);
      if (free == tail) {
        return nullptr;
      }
      while (! firstUnallocated.compare_exchange_weak(free, free + kElementSize * kBlockSize,
                                                      ::std::memory_order_release,
                                                      ::std::memory_order_relaxed
                                                      )
             );
    }
    return free;
  }

  void free (void* p) {
    if (p < data || p >= tail) {
      throw ::std::runtime_error ("Bad free");
    }
    // std::cout << "firstFree:" << (size_t)firstFree.load() << std::endl;
    auto * free = firstFree.load (::std::memory_order_relaxed);
    do {
      *(uint8_t**)p = free;
    } while (! firstFree.compare_exchange_weak(free, (uint8_t *)p,
                                               ::std::memory_order_release,
                                               ::std::memory_order_relaxed
                                               )
             );
    // std::cout << "firstFree:" << (size_t)firstFree.load() << std::endl;
  }
};

TEST (Alloc, Simple) {
  Chunk <10, 3> chunk;
  std::cout << "Size: " << sizeof(chunk) << std::endl;
  auto * a = chunk.allocate();
  std::cout << "a: " << size_t(a) << std::endl;
  a = chunk.allocate();
  std::cout << "a: " << size_t(a) << std::endl;
  a = chunk.allocate();
  std::cout << "a: " << size_t(a) << std::endl;
  chunk.free(a);
  a = chunk.allocate();
  std::cout << "a: " << size_t(a) << std::endl;
  chunk.free(a);
  a = chunk.allocate();
  std::cout << "a: " << size_t(a) << std::endl;
  a = chunk.allocate();
  std::cout << "a: " << size_t(a) << std::endl;
}

