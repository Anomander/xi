#pragma once

#include "xi/ext/configure.h"

namespace xi {
inline namespace util {

  /// explicitly SPSC ring buffer.
  /// this class is not designed to support multiple readers or writers.
  /// with multiple writers there will be a problem of parallel writes to
  /// the same region of the ring before commiting position.
  /// it's possible to work around this by commiting position first in a CAS
  /// loop
  /// and then doing the write, but it brings forth another problem - namely
  /// suspended writers,
  /// when the reading thread arrives at the committed region before the writer
  /// has finished writing.
  /// multiple readers are potentially less damaging and should only cause
  /// re-reads of the same objects.
  /// TODO: the next optimization will be to allow to read batches of items and
  ///       then pop the entire batch in one go.
  template < class T >
  class polymorphic_sp_sc_ring_buffer {
    // make sure indices are never on the same cache line
    alignas(64) atomic< size_t > _write_idx{0};
    alignas(64) atomic< size_t > _read_idx{0};
    // make sure data is independent of indices in cache
    alignas(64) const size_t _SIZE;
    const size_t _RESERVED_SIZE;
    vector< char > _data;

  public:
    polymorphic_sp_sc_ring_buffer(size_t size, size_t reserved = 0)
        : _SIZE(size), _RESERVED_SIZE(reserved), _data(size) {
    }

  protected:
    static size_t write_available(size_t writer_idx,
                                  size_t reader_idx,
                                  size_t size) {
      size_t ret = reader_idx - writer_idx - 1;
      if (XI_UNLIKELY(writer_idx >= reader_idx)) {
        ret += size;
      }
      return ret;
    }

    void *next_readable_block() {
      auto reader_idx = _read_idx.load(memory_order_relaxed); // only modified
                                                              // by reader
                                                              // thread
      auto writer_idx = _write_idx.load(memory_order_acquire);
      do {
        if (reader_idx == writer_idx) {
          return nullptr;
        }
        auto size = *reinterpret_cast< size_t * >(&_data[reader_idx]);
        if (XI_UNLIKELY(0xDEADBEEFFACEC0DE == size ||
                        _SIZE - reader_idx <= sizeof(size_t))) {
          // wrap around
          reader_idx = 0;
          _read_idx.store(reader_idx, memory_order_release);
          continue;
        }
      } while (false);
      return &_data[reader_idx];
    }

    static size_t adjust_for_alignment(
        size_t size, size_t align = alignof(void *)) noexcept {
      return ((size + align - 1) & ~(align - 1));
    }

    template < class U >
    bool do_push(U &&obj, bool force) {
      using UType = decay_t< U >;
      static_assert(is_base_of< T, UType >::value || is_same< T, UType >::value,
                    "U must be a subclass of T");

      struct pod {
        size_t data;
        UType obj;
      };

      auto size                 = adjust_for_alignment(sizeof(pod));
      size_t current_writer_idx = _write_idx.load(memory_order_relaxed);
      size_t reader_idx         = _read_idx.load(memory_order_acquire);
      size_t adjusted_write_idx = current_writer_idx;
      auto available = write_available(current_writer_idx, reader_idx, _SIZE);
      if (XI_UNLIKELY(available < size)) {
        std::cout << __FILE__ << ":"<< __LINE__<< std::endl;
        return false;
      }
      if (adjusted_write_idx + size >= _SIZE) {
        // we need to wrap around
        adjusted_write_idx = 0;
        available = write_available(adjusted_write_idx, reader_idx, _SIZE);
        if (XI_UNLIKELY(available < size)) {
          std::cout << __FILE__ << ":"<< __LINE__<< std::endl;
          std::cout << available << ":"<< size << std::endl;
          // not enough space available after wrap-around
          return false;
        }
      }
      if (XI_UNLIKELY(!force && available - size < _RESERVED_SIZE)) {
        std::cout << __FILE__ << ":"<< __LINE__<< std::endl;
        return false;
      }
      if (adjusted_write_idx < current_writer_idx) {
        // we wrapped around
        if (XI_LIKELY(_SIZE - current_writer_idx > sizeof(size_t))) {
          // we have room to mark
          new (&_data[current_writer_idx]) size_t{0xDEADBEEFFACEC0DE};
        }
      }
      auto *next_block = &_data[adjusted_write_idx];
      // this may throw, but we're okay with it.
      // by this time nothing has been changed irreversibly yet
      new (next_block) pod{size, UType{forward< U >(obj)}};
      _write_idx.store(adjusted_write_idx + size, memory_order_release);
      return true;
    }

  public:
    template < class U >
    bool push(U &&obj) {
      return do_push(forward< U >(obj), false);
    }

    template < class U >
    bool push_forced(U &&obj) {
      return do_push(forward< U >(obj), true);
    }

    T *next() {
      struct pod {
        size_t size;
        char ptr[0];
      };
      auto *p = reinterpret_cast< pod * >(next_readable_block());
      if (XI_UNLIKELY(!p)) {
        return nullptr;
      }
      return reinterpret_cast< T * >(p->ptr);
    }

    void pop() {
      auto reader_idx = _read_idx.load(memory_order_relaxed); // only modified
                                                              // by reader
                                                              // thread
      auto writer_idx = _write_idx.load(memory_order_acquire);
      do {
        if (reader_idx == writer_idx) {
          return;
        }
        auto size = *reinterpret_cast< size_t * >(&_data[reader_idx]);
        if (XI_UNLIKELY(0xDEADBEEFFACEC0DE == size)) {
          reader_idx = 0;
          _read_idx.store(reader_idx, memory_order_release);
          continue;
        } else {
          reinterpret_cast< T * >(&_data[reader_idx + sizeof(size_t)])->~T();
          _read_idx.store(reader_idx +
                              *reinterpret_cast< size_t * >(&_data[reader_idx]),
                          memory_order_release);
        }
      } while (false);
    }
  };
}
}
