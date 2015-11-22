#include "xi/io/buf/chain.h"
#include "xi/io/buf/iterator.h"
#include "xi/io/buf/cursor.h"
#include "xi/io/buf/chain_helper.h"
#include "xi/io/buf/view.h"

namespace xi {
namespace io {

  struct buffer::chain::$ {
    static mut< buffer > release_head(mut< buffer::chain > c) {
      return c->_root._next;
    }
    static mut< buffer > orig(mut< buffer::chain > c) { return edit(c->_root); }
    static mut< buffer > head(mut< buffer::chain > c) { return c->_root._next; }
    static mut< buffer > tail(mut< buffer::chain > c) { return c->_tail; }
    static mut< buffer > prev(mut< buffer > b) { return b->_prev; }
    static mut< buffer > next(mut< buffer > b) { return b->_next; }
    static void reset_buffer_to_tail(mut< buffer > b) {
      b->_begin = b->_arena->data() + b->_offset + b->_capacity;
      b->_end = b->_begin;
    }
    static bool is_chained(mut< buffer > b) { return b->_next != b; }
    static mut< buffer > unlink(mut< buffer > b) {
      if (!is_chained(b)) { return nullptr; }
      return unlink(b, b->_next);
    }

    static mut< buffer > unlink(mut< buffer > beg, mut< buffer > end) {
      if (beg == end) { return nullptr; }
      auto tail = beg->_prev;
      tail->_next = end;
      beg->_prev = end->_prev;
      end->_prev = tail;
      beg->_prev->_next = beg;
      return end;
    }

  };

  void buffer::chain::print() {
    auto p = $::head(this);
    auto i = 0ul;
    while (true) {
      std::cout << (p == $::orig(this)? "H ": "  ") << ++i << ": " << p << "{"<< p->_next<<", "<< p->_prev<<"}"<<std::endl;
      if (p == $::orig(this)) {
        break;
      }
      p = p->_next;
    }
  }
  mut< buffer > buffer::chain::release_head() { return _root._next; }
  mut< buffer > buffer::chain::head() const { return _root._next; }
  mut< buffer > buffer::chain::tail() const { return _tail; }

  buffer::chain::chain() {
    _size = 0;
    _tail = $::head(this);
  }

  buffer::chain::chain(own< buffer > h) {
    _size = helper::subchain_length(edit(h), edit(h));
    helper::insert_after($::head(this), h.release());
    _tail = $::prev($::orig(this));
    while (_tail->size() == 0 && $::prev(_tail)->tailroom() > 0) {
      _tail = $::prev(_tail);
    }
  }

  buffer::chain::~chain() noexcept {
    auto p = $::head(this);
    while (p != $::orig(this)) {
      auto next = $::next(p);
      delete p;
      p = next;
    }
  }

  size_t buffer::chain::headroom() const {
    auto result = 0;
    auto buf = head();
    if (!is_valid(buf)) { return 0; }
    do {
      result += buf->headroom();
      buf = buf->_next;
    } while (buf != head() && buf->is_empty());
    return result;
  }

  size_t buffer::chain::tailroom() const {
    if (!is_valid(_tail)) { return 0; }
    size_t result = _tail->tailroom();
    auto buf = _tail->_next;
    while (buf != head()) {
      result += buf->tailroom();
      buf = buf->_next;
    }
    return result;
  }

  size_t buffer::chain::consume_head(size_t n) {
    while (is_valid(head()) && n >= head()->size()) {
      if (head() == tail()) {
        auto ret = head()->consume_head(n);
        _size -= n - ret;
        return ret;
      }
      n -= head()->size();
      pop(begin());
    }
    if (is_valid(head()) && n > 0) {
      head()->consume_head(n);
      _size -= n;
    }
    return n;
  }

  size_t buffer::chain::consume_tail(size_t n) {
    if (!is_valid(_tail)) { return n; }
    _size -= n;
    while ((n = _tail->consume_tail(n)) > 0) {
      if (XI_UNLIKELY(_tail->_prev == head())) {
        _tail = nullptr;
        return n;
      }
      _tail = _tail->_prev;
    }
    _size += n;
    return n;
  }

  size_t buffer::chain::append_head(size_t n) {
    if (is_valid(head()) && head()->headroom() > 0) {
      _size += n;
      n = head()->append_head(n);
      _size -= n;
    }
    return n;
  }

  size_t buffer::chain::append_tail(size_t n) {
    if (!is_valid(_tail)) { return n; }

    _size += n;
    while ((n = _tail->append_tail(n)) > 0) {
      if (XI_UNLIKELY(_tail->_next == head())) {
        _tail = nullptr;
        break;
      }
      _tail = _tail->_next;
    }
    if (_tail && _tail->tailroom() == 0) {
      _tail = _tail->_next == head() ? nullptr : _tail->_next;
    }
    _size -= n;
    return n;
  }

  void buffer::chain::push_back(chain&& c) {
    _size += c.size();
    helper::insert_after(head(), c.release_head());
    _tail = c._tail;
  }

  void buffer::chain::push_front(chain&& c) {
    _size += c.size();
    helper::insert_before(head(), c.release_head());
  }

  void buffer::chain::push_back(own< buffer > b) {
    assert(!$::is_chained(edit(b)));
    _size += b->size();
    if (XI_UNLIKELY(b->is_empty() && b->tailroom() != b->capacity())) {
      // move pointers around to make room at tail
      helper::reset_buffer(edit(b));
    }
    auto free_b = b.release();
    if (tail() == $::orig(this) && free_b->tailroom() > 0) { _tail = free_b; }
    if (!free_b->is_empty()) {
      _tail = free_b->tailroom() > 0 ? free_b : $::orig(this);
    }
    helper::insert_before($::orig(this), free_b);
  }

  auto buffer::chain::begin() -> iterator {
    return iterator{$::orig(this), $::head(this), 0};
  }

  auto buffer::chain::end() -> iterator {
    return iterator{$::orig(this), $::orig(this), 0};
  }

  own< buffer > buffer::chain::pop(iterator i) {
    assert(i._head == $::orig(this));

    if (i._curr == i._head) { return nullptr; }
    auto next = $::unlink(i._curr);
    if (tail() == i._curr) { _tail = (next == head() ? $::orig(this) : next); }
    _size -= i._curr->size();
    return own< buffer >{i._curr};
  }

  // test when writable_tail is the last to be popped
  // test when writable_tail is in the middle of popped chain
  // test when writable_tail is the head of popped chain
  auto buffer::chain::pop(iterator beg, iterator end) -> chain {
    assert(beg._head == $::orig(this) && beg._head == end._head);

    if (beg == end) { return {}; }
    assert(end > beg); // expensive

    auto next = $::unlink(beg._curr, end._curr);
    chain result{own< buffer >{beg._curr}};
    if (result._tail == _tail) { _tail = (next == head() ? nullptr : next); }
    _size -= result.size();
    return move(result);
  }

  auto buffer::chain::make_cursor(size_t offset) -> cursor {
    return cursor{this, begin() + offset};
  }

  // test when end is first byte of next buffer
  // test when both same buffer
  // test death different heads
  // test allocation size equals correct length
  // test same iterator
  // test iterator reversed
  auto buffer::chain::gather(iterator beg, iterator end,
                             mut< xi::io::buffer_allocator > alloc)
      -> iterator {
    assert(beg._head == end._head);
    auto tail = end.offset_in_curr() ? end : --end;
    if (beg.buf() == tail.buf()) { return beg; }
    if (beg > tail) {
      using std::swap;
      swap(beg, tail);
    }
    auto len = helper::subchain_length(beg.buf(), end.buf());
    auto buffer = alloc->allocate(len);
    cursor c(this, beg);

    // no exceptions below this line
    c.read(buffer->begin(), len - tail.left_in_curr());
    auto next = $::unlink(beg.buf(), tail.buf());
    helper::insert_before(next, buffer.release());
    return iterator(head(), next->_prev, beg.offset_in_curr());
  }

  auto buffer::chain::make_view(iterator beg, iterator end,
                                mut< xi::io::buffer_allocator > alloc) -> view {
    assert(beg._head == end._head);
    auto offset = beg.offset_in_curr();
    auto length = distance(beg, end);
    auto it = gather(beg, end, alloc);
    return it.buf()->make_view(offset, length);
  }
}
}
