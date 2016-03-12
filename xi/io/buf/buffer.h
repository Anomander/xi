// #pragma once

// #include "xi/io/buf/arena_base.h"

// namespace xi {
// namespace io {

//   class buffer : public ownership::unique {
//     own< arena_base > _arena;
//     size_t _offset, _capacity;
//     u8 *_begin, *_end;
//     mut< buffer > _next = this;
//     mut< buffer > _prev = this;

//   public:
//     class view;
//     class chain;

//   private:
//     buffer();

//   public:
//     buffer(own< arena_base >, size_t, size_t);
//     ~buffer();

//   public:
//     u8* begin();
//     u8* end();
//     u8 const* begin() const;
//     u8 const* end() const;
//     pair< u8*, size_t > readable_head() const;
//     pair< u8*, size_t > writable_tail() const;

//     bool is_empty() const;
//     size_t size() const;
//     size_t capacity() const;

//     size_t headroom() const;
//     size_t tailroom() const;

//     size_t consume_head(size_t n);
//     size_t consume_tail(size_t n);
//     size_t append_head(size_t n);
//     size_t append_tail(size_t n);

//     view make_view(size_t offset, size_t length) const;

//   public:
//     void operator delete(void*) {}
//   };

//   inline u8* buffer::begin() { return _begin; }
//   inline u8* buffer::end() { return _end; }

//   inline u8 const* buffer::begin() const {
//     return const_cast< buffer* >(this)->begin();
//   }
//   inline u8 const* buffer::end() const {
//     return const_cast< buffer* >(this)->end();
//   }

//   inline bool buffer::is_empty() const { return _end == _begin; }
//   inline size_t buffer::size() const { return _end - _begin; }
//   inline size_t buffer::capacity() const { return _capacity; }

//   inline size_t buffer::headroom() const {
//     return (is_valid(_arena) ? _begin - _arena->data() + _offset : 0);
//   }
//   inline size_t buffer::tailroom() const {
//     return (is_valid(_arena) ? _arena->data() + _offset + _capacity - _end : 0);
//   }

//   inline size_t buffer::consume_head(size_t n) {
//     auto len = min(n, size());
//     _begin += len;
//     return n - len;
//   }
//   inline size_t buffer::consume_tail(size_t n) {
//     auto len = min(n, size());
//     _end -= len;
//     return n - len;
//   }
//   inline size_t buffer::append_head(size_t n) {
//     auto len = min(n, headroom());
//     _begin -= len;
//     return n - len;
//   }
//   inline size_t buffer::append_tail(size_t n) {
//     auto len = min(n, tailroom());
//     _end += len;
//     return n - len;
//   }
//   inline pair< u8*, size_t > buffer::readable_head() const {
//     return make_pair(_begin, size());
//   }
//   inline pair< u8*, size_t > buffer::writable_tail() const {
//     return make_pair(_end, tailroom());
//   }
// }
// }
