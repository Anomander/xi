#include "xi/io/buf/chain.h"
#include "xi/io/buf/iterator.h"
#include "xi/io/buf/cursor.h"
#include "xi/io/buf/chain_helper.h"
#include "xi/io/buf/view.h"

namespace xi {
namespace io {

  mut< buffer > buffer::chain::release_head() { return _head.release(); }

  void buffer::chain::push_back(chain&& c) {
    _size += c.size();
    if (XI_UNLIKELY(!is_valid(_head))) { _head = move(c._head); } else {
      helper::insert_after(edit(_head), c.release_head());
    }
  }

  void buffer::chain::push_front(chain&& c) {
    _size += c.size();
    if (XI_UNLIKELY(!is_valid(_head))) { _head = move(c._head); } else {
      helper::insert_before(edit(_head), c.release_head());
    }
  }

  void buffer::chain::push_back(own< buffer > b) {
    assert(!helper::is_chained(edit(b)));
    if (!is_valid(_head)) {
      _head = move(b);
      return;
    }
    auto free_b = b.release();
    helper::insert_after(edit(_head), free_b);
    _size += free_b->size();
  }

  auto buffer::chain::begin() -> iterator {
    return iterator{edit(_head), edit(_head), 0};
  }
  auto buffer::chain::end() -> iterator { return iterator{edit(_head)}; }

  own< buffer > buffer::chain::pop(iterator i) {
    assert(i._head == address_of(_head));

    auto next = helper::unlink(i._curr);
    if (address_of(_head) == i._curr) { _head = own< buffer >{next}; }
    _size -= i._curr->size();
    return own< buffer >{i._curr};
  }

  auto buffer::chain::pop(iterator beg, iterator end) -> chain {
    assert(beg._head == address_of(_head) && beg._head == end._head);
    assert(end > beg);

    auto next = helper::unlink(beg._curr, end._curr);
    if (address_of(_head) == beg._curr) { _head = own< buffer >{next}; }
    chain result{own< buffer >{beg._curr}};
    _size -= result.size();
    return move(result);
  }

  buffer::chain::chain(own< buffer > head) : _head(move(head)) {
    _size = helper::subchain_length(edit(_head), edit(_head));
  }

  buffer::chain::~chain() noexcept {
    if (XI_LIKELY(helper::is_chained(edit(_head)))) {
      auto p = _head->_next;
      do {
        auto next = p->_next;
        delete p;
        p = next;
      } while (p != address_of(_head));
    }
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
    auto next = helper::unlink(beg.buf(), tail.buf());
    helper::insert_before(next, buffer.release());
    return iterator(edit(_head), next->_prev, beg.offset_in_curr());
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
