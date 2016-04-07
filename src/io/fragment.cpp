#include "xi/io/fragment.h"

using namespace xi;
using namespace xi::io;

fragment::fragment(byte_blob bytes) noexcept
    : _bytes(move(bytes)), _head(_bytes.begin()), _size(0) {
  maybe_release();
}

void
fragment::maybe_release() {
  if (XI_UNLIKELY(_bytes.empty())) {
    _bytes = {};
    _head  = nullptr;
    _size  = 0;
  }
}

void
fragment::advance(usize sz) {
  assert(sz + size() <= tailroom());

  if (size() > 0) {
    memmove(_head + sz, _head, size());
  }

  _head += sz;
}

void
fragment::retreat(usize sz) {
  assert(sz <= headroom());

  if (size() > 0) {
    memmove(_head, _head + sz, size());
  }

  _head -= sz;
}

usize
fragment::write(const byte_range r) {
  auto cap = min(r.size(), tailroom());
  copy(r.begin(), r.begin() + cap, tail());
  _size += cap;
  return cap;
}

usize
fragment::read(byte_range r, usize offset) {
  if (offset >= size()) {
    return 0;
  }
  auto cap = min(r.size(), size() - offset);
  copy(_head + offset, _head + offset + cap, r.begin());
  return cap;
}

usize
fragment::skip_bytes(usize len) {
  auto cap = min(size(), len);
  _head += cap;
  _size -= cap;
  maybe_release();
  return cap;
}

usize
fragment::ignore_bytes_at_end(usize len) {
  auto cap = min(len, size());
  _size -= cap;
  return cap;
}

own< fragment >
fragment::clone() {
  auto f   = make< fragment >(_bytes.clone());
  f->_head = _head;
  f->_size = _size;
  return f;
}

own< fragment >
fragment::split(usize sz) {
  if (sz >= size()) {
    return nullptr;
  }

  if (!sz && !headroom()) {
    XI_SCOPE(success) {
      _bytes = {};
      _head  = nullptr;
      _size  = 0;
    };
    return make< fragment >(move(_bytes));
  }

  auto split_point = sz + headroom();
  auto ret         = make< fragment >(_bytes.split(split_point));
  ret->_size       = _size - sz;
  _head            = _bytes.begin() + headroom();
  _size            = _bytes.size() - headroom();
  return ret;
}
