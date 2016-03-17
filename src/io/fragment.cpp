#include "xi/io/fragment.h"

using namespace xi;
using namespace xi::io;

fragment::~fragment() {
  _buffer->decrement_ref_count();
}

fragment::fragment(detail::buffer_arena* c, usize offset, usize sz)
    : fragment(INTERNAL, c, c->data + offset, sz, c->data + offset, 0) {
}

fragment::fragment(internal_t, detail::buffer_arena* c, u8* data,
                   usize len, u8* head, usize sz) noexcept : _buffer(c),
                                                             _data(data),
                                                             _length(len),
                                                             _head(head),
                                                             _size(sz) {
  assert(_data >= _buffer->data);
  assert(_data + _length <= _buffer->data + _buffer->length);
  _buffer->increment_ref_count();
}

void fragment::advance(usize sz) {
  assert(sz + size() <= tailroom());

  if (size() > 0) {
    memmove(_head + sz, _head, size());
  }

  _head += sz;
}

void fragment::retreat(usize sz) {
  assert(sz <= headroom());

  if (size() > 0) {
    memmove(_head, _head + sz, size());
  }

  _head -= sz;
}

usize fragment::write(const byte_range r) {
  auto cap = min(r.size(), tailroom());
  copy(r.begin(), r.begin() + cap, tail());
  _size += cap;
  return cap;
}

usize fragment::read(byte_range r) {
  auto cap = min(r.size(), size());
  copy(_head, _head + cap, r.begin());
  return cap;
}

usize fragment::skip_bytes(usize len) {
  auto cap = min(size(), len);
  _head += cap;
  _size -= cap;
  return cap;
}

usize fragment::ignore_bytes_at_end(usize len) {
  auto cap = min(len, size());
  _size -= cap;
  return cap;
}

own< fragment > fragment::clone() {
  return own< fragment >(
      new fragment(INTERNAL, _buffer, _data, _length, _head, _size));
}

own< fragment > fragment::split(usize sz) {
  if (!sz || sz >= size()) {
    return nullptr;
  }

  auto split_point = sz + headroom();
  auto head = _data + split_point;
  auto ret = own< fragment >(new fragment(
      INTERNAL, _buffer, head, _length - split_point, head, size() - sz));
  _size = sz;
  _length = split_point;
  return move(ret);
}
