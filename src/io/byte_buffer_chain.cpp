#include "xi/io/byte_buffer_chain.h"
#include "xi/io/byte_buffer_allocator.h"

#define PRINT //util::print

namespace xi {
namespace io {
  struct byte_buffer::chain::util {
    static void print(mut< byte_buffer > which,
                      mut< byte_buffer > end = nullptr) {
      if (!which) {
        std::cout << "null" << std::endl;
        return;
      }
      if (which == end) { end = which->_next; }
      if (!end) { end = which; }
      do {
        std::cout << (void*)which << "{" << which->headroom() << ","
                  << which->size() << "," << which->tailroom() << "}->";
        which = which->_next;
      } while (which != end);
      std::cout << std::endl;
    }
    static void unlink(mut< byte_buffer > which) {
      which->_next->_prev = which->_prev;
      which->_prev->_next = which->_next;
      which->_prev = which->_next = which;
    }
    static mut< byte_buffer > append_before(mut< byte_buffer > which,
                                            mut< byte_buffer > where) {
      where->_prev->_next = which;
      which->_prev = where->_prev;
      where->_prev = which;
      which->_next = where;
      return which;
    }
    static mut< byte_buffer > append_after(mut< byte_buffer > which,
                                           mut< byte_buffer > where) {
      where->_next->_prev = which;
      which->_next = where->_next;
      where->_next = which;
      which->_prev = where;
      return which;
    }
  };

  byte_buffer::chain::chain(own< byte_buffer > b, usize l, usize s)
      : _head(move(b)), _length(l), _size(s) {
    assert( _head && _length > 0);
  }

  byte_buffer::chain::chain(own< byte_buffer > b) : _head(move(b)) {
    auto n = _head->_next;
    while (head() != n) {
      ++_length;
      _size += n->size();
      n = n->_next;
    }
  }

  void byte_buffer::chain::push_front(own< byte_buffer > p, bool pack) {
    if (!p || (0 == p->size() && 0 == p->tailroom())) return;
    ++_length;
    _size += p->size();
    if (empty()) {
      _head = move(p);
      PRINT(head());
      return;
    }
    auto node = p.get();
    util::append_before(node, _head.release());
    _head = move(p);
    PRINT(head());
  }

  void byte_buffer::chain::push_back(own< byte_buffer > p, bool pack) {
    if (!p || (0 == p->size() && 0 == p->tailroom())) return;
    if (empty()) { return push_front(move(p), pack); }
    ++_length;
    _size += p->size();
    util::append_after(p.release(), tail());
    PRINT(head());
  }

  void byte_buffer::chain::push_front(chain p, bool pack) {
    while (!p.empty()) {
      push_front(p.pop_back(), pack);
    }
  }

  void byte_buffer::chain::push_back(chain p, bool pack) {
    while (!p.empty()) {
      push_back(p.pop_front(), pack);
    }
  }

  own< byte_buffer > byte_buffer::chain::pop(mut< byte_buffer > b) {
    if (empty()) return nullptr;

    --_length;
    _size -= b->size();
    if (b == head()) {
      auto t = tail();
      _head.release();
      if (b != t) { _head = own< byte_buffer >(b->_next); }
    }
    util::unlink(b);
    PRINT(head());
    return own< byte_buffer >(b);
  }

  own< byte_buffer > byte_buffer::chain::pop_front() { return pop(head()); }

  own< byte_buffer > byte_buffer::chain::pop_back() { return pop(tail()); }

  bool byte_buffer::chain::empty() const { return !is_valid(_head); }
  usize byte_buffer::chain::length() const { return _length; }

  usize byte_buffer::chain::compute_writable_size() const {
    if (empty()) return 0;
    usize ret = 0u;
    auto c = tail();
    auto end = head()->_next;
    do {
      ret += c->tailroom();
      if (c->size()) break;
      c = c->_prev;
    } while (c != end);
    return ret;
  }

  auto byte_buffer::chain::split(usize sz)
      -> chain {
    if (XI_UNLIKELY(empty() || 0 == sz)) return {};
    if (XI_UNLIKELY(size() < sz)) {
      auto l = _length, s = _size;
      _length = _size = 0;
      return {move(_head), l, s};
    }
    chain c;
    while (!empty() && sz >= _head->size()) {
      sz -= _head->size();
      c.push_back(pop_front());
    }
    if (sz > 0 && !empty() && XI_UNLIKELY(_head->size() > sz)) {
      auto h = _head->clone();
      _head->discard_read(sz);
      _size -= sz;
      h->discard_write(h->size() - sz);
      c.push_back(move(h));
    }
    return move(c);
  }

  usize byte_buffer::chain::coalesce(mut< byte_buffer_allocator > alloc,
                                     usize sz) {
    if (XI_UNLIKELY(empty() || 0 == sz)) return 0;
    if (XI_UNLIKELY(size() < sz)) {
      sz = size();
    } // only have so much data to coalesce
    if (XI_LIKELY(_head->size() >= sz)) { return _head->size(); }
    if (XI_LIKELY(_head->size() + _head->tailroom() < sz)) {
      // not enough space in head buffer
      push_front(alloc->allocate(sz), false);
    }
    sz -= _head->size();
    while (sz > 0 && head() != _head->_next) {
      auto n = _head->_next;
      auto w = _head->write(byte_range{n->data(), min(sz, n->size())});
      sz -= w;
      n->discard_read(w);
      if (n->empty()) { pop(n); }
    }
    return _head->size();
  }

  usize byte_buffer::chain::read(byte_range r){
    if (empty() || r.empty()) return 0;
    auto data = r.data();
    auto sz = r.size();
    auto c = head();
    do {
      auto read = c->read(byte_range{data, sz});
      sz -= read;
      data += read;
      c = c->_next;
    }
    while (sz && c != head());
    return data - r.data();
  }

  usize byte_buffer::chain::write(const byte_range){
    
  }

  void byte_buffer::chain::trim_start(usize sz) {
    while (! empty() && _head->size() <= sz) {
      sz -= _head->size();
      pop_front();
    }
    if (!empty() && sz) {
      _head->discard_read(sz);
    }
  }

  void byte_buffer::chain::trim_end(usize sz) {
    while (! empty() && tail()->size() <= sz) {
      sz -= tail()->size();
      pop_back();
    }
    if (!empty() && sz) {
      tail()->discard_write(sz);
    }
  }
}
}
