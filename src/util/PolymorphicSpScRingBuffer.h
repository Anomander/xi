#pragma once

#include "ext/Configure.h"

namespace xi {
inline namespace util {

  /// Explicitly SPSC ring buffer.
  /// This class is not designed to support multiple readers or writers.
  /// With multiple writers there will be a problem of parallel writes to
  /// the same region of the ring before commiting position.
  /// It's possible to work around this by commiting position first in a CAS loop
  /// and then doing the write, but it brings forth another problem - namely suspended writers,
  /// when the reading thread arrives at the committed region before the writer
  /// has finished writing.
  /// Multiple readers are potentially less damaging and should only cause
  /// re-reads of the same objects.
  /// TODO: The next optimization will be to allow to read batches of items and
  ///       then pop the entire batch in one go.
  template < class T >
  class PolymorphicSpScRingBuffer {
  private:
    const size_t _SIZE;
    static constexpr size_t PADDINGSIZE = 64 - sizeof(atomic< size_t >);

    atomic< size_t > _writeIdx{0};

    char padding[PADDINGSIZE]; // make sure indices are never on the same cache line

    atomic< size_t > _readIdx{0};
    vector< char > _data;

  public:
    PolymorphicSpScRingBuffer(size_t size) : _SIZE(size), _data(size) {}

  protected:
    static size_t writeAvailable(size_t writerIdx, size_t readerIdx, size_t size) {
      size_t ret = readerIdx - writerIdx - 1;
      if (writerIdx >= readerIdx) {
        ret += size;
      }
      return ret;
    }

    void *nextReadableBlock() {
      auto readerIdx = _readIdx.load(memory_order_relaxed); // only modified by reader thread
      auto writerIdx = _writeIdx.load(memory_order_acquire);
      do {
        if (readerIdx == writerIdx) {
          return nullptr;
        }
        auto size = *reinterpret_cast< size_t * >(&_data[readerIdx]);
        if (0xDEADBEEFFACEC0DE == size || _SIZE - readerIdx <= sizeof(size_t)) {
          // wrap around
          readerIdx = 0;
          _readIdx.store(readerIdx, memory_order_release);
          continue;
        }
      } while (false);
      return &_data[readerIdx];
    }

    static size_t adjustForAlignment(size_t size, size_t align = alignof(void *)) noexcept {
      return ((size + align - 1) & ~(align - 1));
    }

  public:
    template < class U >
    bool push(U &&obj) {
      using UType = decay_t< U >;
      static_assert(is_base_of< T, UType >::value || is_same< T, UType >::value, "U must be a subclass of T");

      struct Pod {
        size_t data;
        UType obj;
      };

      auto size = adjustForAlignment(sizeof(Pod));
      size_t currentWriterIdx = _writeIdx.load(memory_order_relaxed);
      size_t readerIdx = _readIdx.load(memory_order_acquire);
      size_t adjustedWriteIdx = currentWriterIdx;
      auto available = writeAvailable(currentWriterIdx, readerIdx, _SIZE);
      if (available < size) {
        return false;
      }
      if (adjustedWriteIdx + size >= _SIZE) {
        // we need to wrap around
        adjustedWriteIdx = 0;
        available = writeAvailable(adjustedWriteIdx, readerIdx, _SIZE);
        if (available < size) {
          // not enough space available after wrap-around
          return false;
        }
      }
      if (adjustedWriteIdx < currentWriterIdx) {
        // we wrapped around
        if (_SIZE - currentWriterIdx > sizeof(size_t)) {
          // we have room to mark
          new (&_data[currentWriterIdx]) size_t{0xDEADBEEFFACEC0DE};
        }
      }
      auto *nextBlock = &_data[adjustedWriteIdx];
      if (!nextBlock) {
        return false;
      }
      // this may throw, but we're okay with it.
      // by this time nothing has been changed irreversibly yet
      new (nextBlock) Pod{size, UType{forward< U >(obj)}};
      _writeIdx.store(adjustedWriteIdx + size, memory_order_release);
      return true;
    }

    T *next() {
      struct Pod {
        size_t size;
        char ptr[0];
      };
      auto *pod = reinterpret_cast< Pod * >(nextReadableBlock());
      if (!pod) {
        return nullptr;
      }
      return reinterpret_cast< T * >(pod->ptr);
    }

    void pop() {
      auto readerIdx = _readIdx.load(memory_order_relaxed); // only modified by reader thread
      auto writerIdx = _writeIdx.load(memory_order_acquire);
      do {
        if (readerIdx == writerIdx) {
          return;
        }
        auto size = *reinterpret_cast< size_t * >(&_data[readerIdx]);
        if (0xDEADBEEFFACEC0DE == size) {
          readerIdx = 0;
          _readIdx.store(readerIdx, memory_order_release);
          continue;
        } else {
          reinterpret_cast< T * >(&_data[readerIdx + sizeof(size_t)])->~T();
          _readIdx.store(readerIdx + *reinterpret_cast< size_t * >(&_data[readerIdx]), memory_order_release);
        }
      } while (false);
    }
  };
}
}