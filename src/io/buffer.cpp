#include "xi/ext/configure.h"
#include "xi/io/buffer.h"
#include "xi/io/buffer_allocator.h"
#include "xi/io/detail/buffer_utils.h"

namespace xi {
namespace io {

  buffer::buffer(own< fragment > b) {
    if (!b->size() && !b->headroom() && !b->tailroom()) {
      return;
    }
    assert(b);
    _size = b->size();
    _fragments.push_back(*b.release());
  }

  buffer::buffer(list_t &&fragments, usize sz)
      : _fragments(move(fragments)), _size(sz) {
  }

  buffer::~buffer() {
    _size = 0;
    _fragments.clear_and_dispose(fragment_deleter);
  }

  void buffer::push_front(own< fragment > &&p, bool pack) {
    if (!p) {
      return;
    }
    _size += p->size();
    _fragments.insert(begin(_fragments), *p.release());
  }

  void buffer::push_back(own< fragment > &&p, bool pack) {
    if (!p) {
      return;
    }
    _size += p->size();
    _fragments.insert(end(_fragments), *p.release());
  }

  void buffer::push_front(own< buffer > &&p, bool pack) {
    if (!p) {
      return;
    }
    _size += p->size();
    _fragments.splice(begin(_fragments), p->_fragments);
  }

  void buffer::push_back(own< buffer > &&p, bool pack) {
    if (!p) {
      return;
    }
    _size += p->size();
    _fragments.splice(end(_fragments), p->_fragments);
  }

  bool buffer::empty() const {
    return _fragments.empty();
  }

  usize buffer::headroom() const {
    if (empty()) {
      return 0;
    }
    return begin(_fragments)->headroom();
  }

  usize buffer::tailroom() const {
    if (empty()) {
      return 0;
    }
    return prev(end(_fragments))->tailroom();
  }

  own< buffer > buffer::split(usize sz) {
    if (XI_UNLIKELY(empty() || 0 == sz)) {
      return {};
    }
    if (XI_UNLIKELY(size() < sz)) {
      auto old_size = _size;
      _size         = 0;
      return make< buffer >(move(_fragments), old_size);
    }

    auto remainder = sz;
    auto it = skip_offset(begin(_fragments), end(_fragments), edit(remainder));

    if (XI_LIKELY(remainder > 0)) {
      auto new_node = it->split(remainder);
      it            = _fragments.insert(++it, *new_node.release());
    }
    list_t new_list;
    for (auto i = _fragments.begin(); i != it;) {
      _size -= i->size();
      new_list.splice(new_list.end(), _fragments, i++);
    }

    return make< buffer >(move(new_list), sz);
  }

  usize buffer::coalesce(mut< buffer_allocator > alloc,
                         usize offset,
                         usize length) {
    if (XI_UNLIKELY(empty() || 0 == length || offset >= size())) {
      return 0;
    }

    if (length > _size - offset) {
      length = _size - offset;
    }

    auto end = _fragments.end();
    auto it  = skip_offset(begin(_fragments), end, edit(offset));

    auto head_size = it->size() - offset;
    if (XI_UNLIKELY(head_size >= length)) {
      // the first buffer has enough contiguous space
      return _size;
    }

    list_t::iterator write_fragment_it = it;
    if (XI_LIKELY(head_size + it->tailroom() < length)) {
      // not enough space in the first buffer
      auto new_buffer = alloc->allocate(length);
      _fragments.splice(it, new_buffer->_fragments);
      write_fragment_it = prev(it);
    } else {
      // skip the buffer we'll write into
      ++it;
    }

    auto remove_start = it;
    length -= write_fragment_it->size();
    for (; it != end && length >= it->size(); ++it) {
      // copy data from coalesced buffers
      length -= write_fragment_it->write(it->data_range());
    }
    _fragments.erase_and_dispose(remove_start, it, fragment_deleter);
    if (length > 0) {
      // partially coalesced buffer
      write_fragment_it->write(it->data_range().subrange(0, length));
      it->skip_bytes(length);
    }
    return write_fragment_it->size();
  }

  own< buffer > buffer::clone() {
    list_t fragments;
    for (auto &&f : _fragments) {
      fragments.push_back(*f.clone().release());
    }
    return make< buffer >(move(fragments), _size);
  }

  byte_range buffer::range(mut< buffer_allocator > alloc,
                           usize offset,
                           usize length) {
    if (offset >= _size) {
      return byte_range::null();
    }
    coalesce(alloc, offset, length);

    auto it = skip_offset(begin(_fragments), end(_fragments), edit(offset));
    return byte_range{it->data() + offset, min(length, it->size() - offset)};
  }

  usize buffer::read(byte_range r, usize offset) {
    if (XI_UNLIKELY(empty() || r.empty() || offset >= _size)) {
      return 0;
    }

    auto end = _fragments.end();
    auto it  = skip_offset(begin(_fragments), end, edit(offset));

    auto sz = r.size();
    while (it != end) {
      auto ret = it->read(r, offset);
      offset   = 0;
      r        = r.subrange(ret);
      if (r.empty()) {
        break;
      }
      ++it;
    }
    return sz - r.size();
  }

  usize buffer::write(const byte_range r) {
    if (XI_UNLIKELY(empty() || r.empty())) {
      return 0;
    }

    auto ret = prev(end(_fragments))->write(r);
    _size += ret;
    return ret;
  }

  void buffer::skip_bytes(usize sz, bool free) {
    if (XI_UNLIKELY(empty())) {
      return;
    }

    auto remainder = sz;
    auto beg       = _fragments.begin();
    auto end       = _fragments.end();
    auto it        = skip_offset(beg, end, edit(remainder));

    if (XI_UNLIKELY(it == end)) {
      _size = 0;
    } else {
      it->skip_bytes(remainder);
      if (it->empty()) {
        ++it;
      }
      _size -= sz;
    }

    if (free) {
      _fragments.erase_and_dispose(beg, it, fragment_deleter);
    } else {
      for_each(beg, it, [](auto &i) { i.skip_bytes(i.size()); });
    }
  }

  void buffer::ignore_bytes_at_end(usize sz, bool free) {
    if (XI_UNLIKELY(empty())) {
      return;
    }

    auto remainder = sz;
    auto beg       = _fragments.rbegin();
    auto end       = _fragments.rend();
    auto it        = skip_offset(beg, end, edit(remainder));

    if (XI_UNLIKELY(it == end)) {
      _size = 0;
    } else {
      it->ignore_bytes_at_end(remainder);
      _size -= sz;
    }
    if (free) {
      _fragments.erase_and_dispose(
          it.base(), _fragments.end(), fragment_deleter);
    } else {
      for_each(beg, it, [](auto &i) { i.ignore_bytes_at_end(i.size()); });
    }
  }
}
}
