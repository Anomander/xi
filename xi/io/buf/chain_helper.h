#pragma once

#include "xi/io/buf/chain.h"
#include "xi/io/buf/iterator.h"

namespace xi {
namespace io {

  struct buffer::chain::helper {
    static bool is_chained(mut< buffer > b) { return b->_next != b; }
    static void insert_before(mut< buffer > a, mut< buffer > b) {
      insert_after(a->_prev, b);
    }
    static void insert_after(mut< buffer > a, mut< buffer > b) {
      insert_chain_after(a, b, b->_prev);
    }
    static void insert_chain_after(mut< buffer > a, mut< buffer > begin,
                                   mut< buffer > end) {
      a->_next->_prev = end;
      end->_next = a->_next;
      a->_next = begin;
      begin->_prev = a;
    }

    static mut< buffer > unlink(mut< buffer > b) {
      if (!is_chained(b)) { return nullptr; }
      return unlink(b, b->_next);
    }

    static mut< buffer > unlink(mut< buffer > beg, mut< buffer > end) {
      if (beg == end) { return nullptr; }
      beg->_prev->_next = end;
      end->_prev = beg->_prev;
      beg->_prev = end->_prev;
      beg->_prev->_next = beg;
      return end;
    }

    static size_t subchain_length(mut< buffer > beg, mut< buffer > end) {
      auto p = beg->_next;
      auto result = beg->size();
      while (p != end) {
        result += p->size();
        p = p->_next;
      }
      return result;
    }
  };

}
}
