// #pragma once

// #include "xi/ext/configure.h"
// #include "xi/io/data_message.h"

// namespace xi {
// namespace io {

//   class stream_buffer {
//     enum { kPageSize = 4096, kChunkSize = 8 * kPageSize };
//     struct chunk {
//       uint8_t *data = new uint8_t[kChunkSize];
//       uint8_t *w_cursor = data;
//       uint8_t *r_cursor = data;

//       ~chunk() { delete[] data; }
//       uint8_t *begin() const noexcept { return data; }
//       uint8_t *end() const noexcept { return data + kChunkSize; }
//       size_t size() const { return w_cursor - r_cursor; }
//       size_t capacity() const { return end() - w_cursor; }

//       void consume(size_t sz) {
//         auto actual = min(size(), sz);
//         r_cursor += actual;
//       }
//       void push(mut< byte_range > range) {
//         auto actual = min(capacity(), range->size());
//         ::memcpy(w_cursor, range->data(), actual);
//         w_cursor += actual;
//         range->consume(actual);
//       }
//     };
//     size_t _total_size = 0UL;
//     size_t _free_capacity = 0UL;
//     deque< chunk > _chunks;
//     deque< chunk >::iterator _write_cursor = _chunks.end();

//   public:
//     /// exception safety: strong; either the entire range is pushed or nothing
//     /// is
//     /// pushed
//     void push(byte_range range) {
//       if (range.empty()) { return; }
//       allocate_capacity(range.size);
//       copy_no_allocate(range);
//     }

//     bool empty() const { return _total_size == 0; }
//     size_t size() const { return _total_size; }

//     size_t fill_vec(byte_range *vec, size_t max) const {
//       if (empty()) { return 0; }
//       auto it = begin(_chunks);
//       size_t i = 0;
//       for (; i < max && it != end(_chunks); ++i, ++it) {
//         if (it->size() == 0) { break; }
//         vec[i].data = it->r_cursor;
//         vec[i].size = it->size();
//       }
//       return i;
//     }

//     void consume(size_t size) {
//       auto it = begin(_chunks);
//       while (it->size() <= size && it != _write_cursor) {
//         size -= it->size();
//         ++it;
//       }
//       _chunks.erase(begin(_chunks), it);
//       it->consume(size);
//       _total_size -= min(size, _total_size);
//     }

//   private:
//     void allocate_capacity(size_t needed) {
//       if (_write_cursor == end(_chunks)) {
//         _chunks.emplace_back();
//         _write_cursor = prev(end(_chunks));
//         _free_capacity += _write_cursor->capacity();
//       }
//       while (_free_capacity < needed) {
//         _chunks.emplace_back();
//         _free_capacity += _chunks.back().capacity();
//       }
//     }

//     void copy_no_allocate(byte_range range) {
//       _free_capacity -= range.size;
//       do {
//         _total_size += range.size;
//         _write_cursor->push(edit(range));
//       } while (!range.empty() && ++_write_cursor != end(_chunks));
//     }
//   };
// }
// }
