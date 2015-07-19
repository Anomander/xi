#pragma once

#include "ext/Configure.h"

namespace xi {
inline namespace util {

template < size_t N > struct BytesStored {
  template < class T > static constexpr size_t Count() { return N; }
};
template < size_t N > struct ObjectsStored {
  template < class T > static constexpr size_t Count() {
    return (sizeof(T) + alignof(T *) - sizeof(T) % alignof(T *)) * N;
  }
};

struct Foo {
  char bar[23];
};
static_assert(ObjectsStored< 1 >::Count< Foo >() == 24, "");
static_assert(ObjectsStored< 16 >::Count< Foo >() == 16 * 24, "");

struct NoLockStrategy {
  auto lock() { struct EmptyLock { };
    return EmptyLock{};
  }
};

template < class T, size_t N, class LockStrategy = NoLockStrategy > class PreallocatedObjectPool {
  aligned_storage_t< sizeof(T) > _data[N];
  LockStrategy _lockStrategy;

  array< T *, N > _freelist[N] = { nullptr };
  size_t _numFree = 0;

public:
  T *allocate(size_t count) {
    T *result = nullptr;
    auto lock = _lockStrategy.lock();
    if (_numFree) {
      result = _freelist[--_numFree];
    }
    return result;
  }
  void free(T *) { auto lock = _lockStrategy.lock(); }
};

template < size_t N, size_t DefaultChunkSize = N * 64, size_t InitialChunks = 2 > class BlockAllocator {
  struct Chunk {
    static constexpr uint8_t *kEof = (uint8_t *)1;
    Chunk *next;
    atomic< size_t > count;
    atomic< uint8_t * > firstFree{ kEof };
    atomic< uint8_t * > firstUnallocated;

    aligned_storage_t< sizeof(uint8_t[N]), alignof(uint8_t[N]) > data[DefaultChunkSize];
    char tail[0];

    Chunk() { firstUnallocated.store(data, ::std::memory_order_relaxed); }
    void *allocate() {
      auto *free = firstFree.load(::std::memory_order_relaxed);
      auto *nextFree = kEof;
      do {
        if (kEof == free) {
          break;
        }
        nextFree = *(uint8_t **)free;
      } while (
          !firstFree.compare_exchange_weak(free, nextFree, ::std::memory_order_release, ::std::memory_order_relaxed));
      free = firstUnallocated.load(::std::memory_order_relaxed);
      if (free == tail) {
        return next->allocate();
      }
      while (!firstUnallocated.compare_exchange_weak(free, free + N, ::std::memory_order_release,
                                                     ::std::memory_order_relaxed))
        ;
      return free;
    }
  };

public:
  void *allocate() { return nullptr; }
};
}
}
