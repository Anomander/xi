#include "xi/io/byte_buffer.h"

using namespace xi;
using namespace xi::io;

byte_buffer::~byte_buffer() {
  while (_next != this) {
    auto n = _next->_next;
    _next->_next = _next->_prev = _next;
    delete _next;
    _next = n;
  }
  _buffer->decrement_ref_count();
}

byte_buffer::byte_buffer(detail::byte_buffer_arena* c, usize offset, usize sz)
    : byte_buffer(kInternal, c, c->data + offset, sz, c->data + offset, 0) {}

byte_buffer::byte_buffer(internal_t, detail::byte_buffer_arena* c, u8* data,
                         usize len, u8* head, usize sz) noexcept : _buffer(c),
                                                                   _data(data),
                                                                   _length(len),
                                                                   _head(head),
                                                                   _size(sz) {
  assert(_data >= _buffer->data);
  assert(_data + _length <= _buffer->data + _buffer->length);
  _buffer->increment_ref_count();
}

void byte_buffer::advance(usize sz) {
  assert(sz + size() <= tailroom());

  if (size() > 0) { memmove(_head + sz, _head, size()); }

  _head += sz;
}

void byte_buffer::retreat(usize sz) {
  assert(sz <= headroom());

  if (size() > 0) { memmove(_head, _head + sz, size()); }

  _head -= sz;
}

usize byte_buffer::write(const byte_range r) {
  auto cap = min(r.size(), tailroom());
  copy(r.begin(), r.begin() + cap, tail());
  _size += cap;
  return cap;
}

usize byte_buffer::read(byte_range r) {
  auto cap = min(r.size(), size());
  copy(_head, _head + cap, r.begin());
  return cap;
}

usize byte_buffer::discard_read(usize len) {
  auto cap = min(size(), len);
  _head += cap;
  _size -= cap;
  return cap;
}

usize byte_buffer::discard_write(usize len) {
  auto cap = min(len, size());
  _size -= cap;
  return cap;
}

own< byte_buffer > byte_buffer::clone() {
  return own< byte_buffer >(
      new byte_buffer(kInternal, _buffer, _data, _length, _head, _size));
}
