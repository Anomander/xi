#pragma once

#include <netinet/in.h>
#include "xi/ext/configure.h"

namespace xi {
namespace io {
    namespace detail {
      constexpr size_t aligned_size2(size_t sz, size_t align = alignof(void*)) {
        return (sz + align - 1) & ~(align - 1);
      }
    }

    struct byte_buf;
    struct chunk;

    struct chunk_deallocator : public virtual ownership::std_shared {
      virtual ~chunk_deallocator() = default;
      virtual void deallocate(chunk*) noexcept = 0;
    };

    struct chunk {
      usize length;
      atomic< u64 > ref_count{0};
      own< chunk_deallocator > deallocator;
      u8 data[0];

      chunk(usize l, own< chunk_deallocator > d)
          : length(l), deallocator(move(d)) {}

      void increment_ref_count() noexcept {
        ref_count.fetch_add(1, memory_order_relaxed);
      }
      void decrement_ref_count() noexcept {
        if (1 >= ref_count.fetch_sub(1, memory_order_relaxed)) {
          auto dealloc = deallocator;
          this->~chunk();
          dealloc->deallocate(this);
        }
      }
    };
    static_assert(sizeof(chunk) == 32, "");

    struct byte_buf : public virtual ownership::unique {
      enum internal_t { kInternal };

      chunk* _buffer;
      u8* _data;
      usize _length;
      u8* _write_cursor;
      byte_buf* _next = this;
      byte_buf* _prev = this;

    public:
      void operator delete(void* p) {
        static_cast< byte_buf* >(p)->decrement_ref();
      }

      struct iovec_adapter;
      class chain;

    private:
      template < class > friend class basic_byte_buf_allocator;

      byte_buf(chunk* c, usize offset, usize sz)
          : byte_buf(kInternal, c, c->data + offset, sz) {}

      byte_buf(internal_t, chunk* c, u8* data, usize sz) noexcept
          : _buffer(c),
            _data(data),
            _length(sz),
            _write_cursor(_data) {
        assert(_data >= _buffer->data);
        assert(_data + _length <= _buffer->data + _buffer->length);
        _buffer->increment_ref_count();
      }

      XI_NEITHER_MOVABLE_NOR_COPIABLE(byte_buf);

      void decrement_ref() noexcept {
        while (_next != this) {
          auto n = _next->_next;
          delete _next;
          _next = n;
        }
        _buffer->decrement_ref_count();
      }

    public:
      u8* tail() const { return _data + _length; }
      usize writable_size() const { return tail() - _write_cursor; }
      usize readable_size() const { return _write_cursor - _data; }

      usize write(u8* begin, usize len) {
        auto cap = min(len, writable_size());
        copy(begin, begin + cap, _write_cursor);
        _write_cursor += cap;
        return cap;
      }

      usize read(u8* begin, usize len) {
        auto cap = min(len, readable_size());
        copy(_data, _data + cap, begin);
        return cap;
      }

      usize discard_read(usize len) {
        auto old_size = readable_size();
        if (len >= old_size) {
          _write_cursor = _data;
          return old_size;
        }
        auto remainder = old_size - len;
        memmove(_data, _data + len, remainder);
        _write_cursor = _data + len;
        return len;
      }

      usize discard_write(usize len) {
        auto cap = min(len, readable_size());
        _write_cursor -= cap;
        return cap;
      }

      unique_ptr< byte_buf > split(usize offset) {
        // not enough space for buffer metadata
        if (offset > writable_size() ||
            writable_size() - offset <= sizeof(byte_buf))
          return nullptr;

        auto location = _write_cursor + offset;
        auto length = writable_size() - offset - sizeof(byte_buf);

        _length -= length + sizeof(byte_buf);

        auto p = new (location)
            byte_buf{kInternal, _buffer, location + sizeof(byte_buf), length};
        return unique_ptr< byte_buf >(p);
      }
    };
    static_assert(sizeof(byte_buf) == 56, "");

    class byte_buf::chain {
      own< byte_buf > _head;
      mut< byte_buf > _tail;

    public:
      XI_ONLY_MOVABLE(chain);

      mut< byte_buf > head() { return edit(_head); }
      mut< byte_buf > tail() { return _tail; }

      void append(own< byte_buf > p) {
        if (!p) return;
        auto old_next = _tail->_next;
        _tail->_next = p.release();
        old_next->_prev = _tail->_next;
        _tail->_next->_next = old_next;
        _tail->_next->_prev = edit(_head);
      }

      void prepend(own< byte_buf > p) {
        if (!p) return;
        auto old_prev = _head->_prev;
        _head->_prev = p.release();
        old_prev->_next = _head->_prev;
        _head->_prev->_next = edit(_head);
        _head->_prev->_prev = old_prev;
      }
    };

    struct byte_buf::iovec_adapter {
      static usize report_written(mut< byte_buf > b, usize sz) {
        auto change = min(b->writable_size(), sz);
        b->_write_cursor += change;
        return change;
      }
      static void fill_writable_iovec(mut< byte_buf > b, mut< iovec > iov) {
        iov->iov_base = b->_write_cursor;
        iov->iov_len = b->writable_size();
      }
      static void fill_readable_iovec(mut< byte_buf > b, mut< iovec > iov) {
        iov->iov_base = b->_data;
        iov->iov_len = b->readable_size();
      }
    };

    class heap_storage_allocator : public virtual ownership::std_shared {
    public:
      void* allocate(usize sz) {
        auto m = malloc(sz);
        std::cout << "malloc(" << m << ")" << std::endl;
        return m;
      }
      void free(void* p) {
        std::cout << "free(" << p << ")" << std::endl;
        ::free(p);
      }
    };

    struct byte_buf_allocator : public virtual ownership::std_shared {
      virtual ~byte_buf_allocator() = default;
      virtual own< byte_buf > allocate(usize) = 0;
    };

    template < class SA >
    class basic_byte_buf_allocator : public byte_buf_allocator,
                               public chunk_deallocator {
      own< SA > _storage_alloc = make< SA >();

    public:
      basic_byte_buf_allocator() = default;
      basic_byte_buf_allocator(own< SA > storage_alloc)
          : _storage_alloc(move(storage_alloc)) {}

      chunk* allocate_chunk(usize sz) {
        auto size = sz + detail::aligned_size2(sizeof(chunk));
        return new (_storage_alloc->allocate(size)) chunk{sz, share(this)};
      };

      own< byte_buf > allocate(usize sz) override {
        auto overhead = detail::aligned_size2(sizeof(byte_buf));
        auto c = allocate_chunk(sz + overhead);
        XI_SCOPE(fail) { _storage_alloc->free(c); };
        return own< byte_buf >{new (c->data) byte_buf(c, overhead, sz)};
      }
      void deallocate(chunk* c) noexcept override { _storage_alloc->free(c); }
    };

    using pooled_byte_buf_allocator =
        basic_byte_buf_allocator< heap_storage_allocator >;

//   namespace buf {

//     class block {
//       const u64 _capacity = 0;
//       u8* _rcursor = _data;
//       u8* _wcursor = _data;
//       block* _next = nullptr;
//       u8 _data[0];

//     public:
//       class iovec_adapter;

//       block(u64 s) : _capacity(s) {}
//       virtual ~block() = default;

//       u64 size() const { return static_cast< u64 >(_wcursor - _rcursor); }
//       u64 capacity() const { return _capacity - size(); }
//       u8 const* data() const { return _data; }
//       u8* rcursor() const { return _rcursor; }
//       u8* wcursor() const { return _wcursor; }
//       u8 const* end() const { return data() + _capacity; }
//       u8* end() { return wcursor() + capacity(); }

//       bool spent() const { return _rcursor == end(); }
//       block* next() const { return _next; }
//       block* append(block* b) {
//         auto n = this;
//         while (n->_next) { n = n->_next; }
//         return n->_next = move(b);
//       }
//       u64 write(u8* begin, u8* end) {
//         auto cap = min< u64 >(distance(begin, end), capacity());
//         copy(begin, begin + cap, _wcursor);
//         _wcursor += cap;
//         if (_next && !capacity()) {
//           return cap + _next->write(begin + cap, end);
//         }
//         return cap;
//       }
//       u64 read(u8* begin, u8* end) {
//         auto cap = min< u64 >(distance(begin, end), size());
//         copy(_rcursor, _rcursor + cap, begin);
//         begin += cap;
//         if (_next && begin != end) { return cap + _next->read(begin, end); }
//         return cap;
//       }
//       u64 consume(u64 count) {
//         auto cap = min< u64 >(count, size());
//         _rcursor += cap;
//         return cap;
//       }
//     };

//     class allocator : public virtual ownership::std_shared {
//     public:
//       virtual block* allocate(size_t sz) = 0;
//       virtual void deallocate(block* b) = 0;

//     protected:
//       virtual ~allocator() = default;
//     };

//     struct heap_allocator : public allocator {
//       u64 active = 0;

//       block* allocate(size_t sz) override {
//         auto mem = malloc(sz + sizeof(block));
//         std::cout << "(" << ++active << ") Allocated 1 block of " << sz << " @ "
//                   << mem << std::endl;
//         return new (mem) block{sz};
//       }
//       void deallocate(block* b) override {
//         std::cout << "(" << --active << ") Deallocated 1 block @ " << (void*)b
//                   << std::endl;
//         b->~block();
//         free(b);
//       }
//     };

//     class buf {
//       own< allocator > _alloc;

//     public:
//       class iovec_adapter;

//       buf(own< allocator > alloc) : _alloc(move(alloc)) {}
//       ~buf() {
//         while (_head) {
//           auto next = _head->next();
//           _alloc->deallocate(_head);
//           _head = next;
//         }
//       }
//       void expand(u64 sz) {
//         _capacity += sz;
//         if (!_head) { _tail = _head = _alloc->allocate(sz); } else {
//           _tail = _tail->append(_alloc->allocate(sz));
//         }
//         if (!_cursor) { _cursor = _tail; }
//       }
//       // buf extract(u64, mut< allocator >);
//       // view extract_view(u64, mut< allocator >);

//       u64 size() const { return _size; }
//       u64 capacity() const { return _capacity; }
//       bool empty() const { return size() == 0U && capacity() == 0U; }

//       u64 write(void* data, u64 sz) {
//         if (!capacity() || !_head) return 0;
//         auto bytes = reinterpret_cast< u8* >(data);
//         auto ret = _cursor->write(bytes, bytes + sz);
//         _capacity -= ret;
//         _size += ret;
//         return ret;
//       }

//       u64 read(void* data, u64 sz) {
//         if (!size() || !_head) return 0;
//         auto bytes = reinterpret_cast< u8* >(data);
//         return _head->read(bytes, bytes + sz);
//       }

//       u64 consume(u64 count) {
//         if (!size() || !_head) return 0;
//         u64 remaining = count;
//         while (_head) {
//           remaining -= _head->consume(remaining);
//           if (_head->spent()) {
//             auto p = _head;
//             _head = _head->next();
//             _alloc->deallocate(p);
//           } else { break; }
//         }
//         auto ret = count - remaining;
//         _size -= ret;
//         return ret;
//       }

//       u64 read_consume(void* data, u64 sz) { return consume(read(data, sz)); }

//       void reserve(u64 size) {
//         if (capacity() >= size) return;
//         expand(size - capacity());
//       }

//     private:
//       block* _head = nullptr;
//       mut< block > _cursor = nullptr;
//       mut< block > _tail = nullptr;
//       u64 _capacity = 0;
//       u64 _size = 0;
//     };

//     class block::iovec_adapter {
//     public:
//       static u64 report_written(mut< block > b, u64 sz) {
//         auto change = min(b->capacity(), sz);
//         b->_wcursor += change;
//         return change;
//       }
//       static void fill_writable_iovec(mut< block > b, mut< iovec > iov) {
//         iov->iov_base = b->_wcursor;
//         iov->iov_len = b->capacity();
//       }
//       static void fill_readable_iovec(mut< block > b, mut< iovec > iov) {
//         iov->iov_base = b->_rcursor;
//         iov->iov_len = b->size();
//       }
//     };

//     class buf::iovec_adapter {
//     public:
//       static void report_written(mut< buf > b, u64 sz) {
//         if (!b->capacity() || !b->_cursor) return;
//         auto c = b->_cursor;
//         auto count = sz;
//         while (c && count > 0) {
//           count -= block::iovec_adapter::report_written(c, count);
//           if (!c->capacity()) { b->_cursor = b->_cursor->next(); }
//           c = c->next();
//         }
//         b->_size += (sz - count);
//         b->_capacity -= (sz - count);
//       }
//       static u64 fill_writable_iovec(mut< buf > b, mut< iovec > iov, u64 sz,
//                                      u64 skip = 0, u64 start = 0) {
//         auto c = b->_cursor;
//         auto cnt = start;
//         while (c && cnt < sz) {
//           if (skip) { --skip; } else if (c->capacity() > 0) {
//             block::iovec_adapter::fill_writable_iovec(c, edit(iov[cnt++]));
//           }
//           c = c->next();
//         }
//         return cnt;
//       }
//       static u64 fill_readable_iovec(mut< buf > b, mut< iovec > iov, u64 sz,
//                                      u64 skip = 0, u64 start = 0) {
//         auto c = b->_head;
//         auto cnt = start;
//         while (c && cnt < sz) {
//           if (skip) { --skip; } else if (c->size() > 0) {
//             block::iovec_adapter::fill_readable_iovec(c, edit(iov[cnt++]));
//           }
//           c = c->next();
//         }
//         return cnt;
//       }
//     };
//   }
}
}
