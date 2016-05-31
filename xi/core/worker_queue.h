#pragma once

#include "xi/ext/configure.h"
#include "xi/core/detail/intrusive.h"
#include "xi/core/resumable.h"

namespace xi {
namespace core {
  namespace v2 {

    class worker_queue {
      core::detail::ready_queue_type<resumable> _queue;

    public:
      void enqueue(own< resumable >);
      opt< own< resumable > > dequeue();
      bool is_empty() const;
    };

    inline void worker_queue::enqueue(own< resumable > r) {
      assert(nullptr != r);
      _queue.push_back(*(r.release()));
    }

    inline opt< own< resumable > > worker_queue::dequeue() {
      if (is_empty()) {
        return none;
      }
      XI_SCOPE(exit) {
        _queue.pop_front();
      };
      return some(own< resumable >{&_queue.front()});
    }

    inline bool worker_queue::is_empty() const {
      return _queue.empty();
    }
  }
}
}
