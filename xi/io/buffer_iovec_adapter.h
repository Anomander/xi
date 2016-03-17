#pragma once

#include "xi/io/buffer.h"
#include <netinet/in.h>
#include <limits.h>

namespace xi {
namespace io {

  class buffer::iovec_adapter {
  public:
    static usize fill_readable_iovec(ref< buffer > b, iovec* iov, u16 max) {
      assert(max <= IOV_MAX);
      if (b.empty() || 0 == b.size() || 0 == max) {
        return 0;
      }
      u16 i = 0;
      for (auto& f : b._fragments) {
        auto range = f.data_range();
        if (range.empty()) {
          continue;
        }
        iov[i].iov_base = range.data();
        iov[i].iov_len = range.size();
        ++i;
        if (max == i) {
          break;
        }
      }
      return i;
    }
    static usize fill_writable_iovec(mut< buffer > b, iovec* iov) {
      if (b->empty()) {
        return 0;
      }
      auto range = prev(end(b->_fragments))->tail_range();
      if (range.empty()) {
        return 0;
      }
      iov[0].iov_base = range.data();
      iov[0].iov_len = range.size();
      return 1;
    }
    static void report_written(mut< buffer > b, usize sz) {
      if (b->empty()) {
        return;
      }
      b->_size += (prev(end(b->_fragments))->record_bytes(sz));
    }
  };
}
}
