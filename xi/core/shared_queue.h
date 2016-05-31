#pragma once

#include "xi/ext/configure.h"
#include "xi/ext/lockfree.h"
#include "xi/core/resumable.h"
#include "xi/core/resumable_builder.h"
#include "xi/core/worker_queue.h"

namespace xi {
namespace core {
  namespace v2 {

    class shared_queue : public ownership::unique {
      enum { INITIAL_CAPACITY = 256 };
      lockfree::queue< resumable_builder* > _queue{INITIAL_CAPACITY};

    public:
      void enqueue(own< resumable_builder >);
      usize dequeue_into(mut< worker_queue >, usize n);
      bool is_empty() const;
    };

    inline void shared_queue::enqueue(own< resumable_builder > r) {
      assert(nullptr != r);
      _queue.push(r.release());
    }

    inline usize shared_queue::dequeue_into(mut< worker_queue > q, usize n) {
      for ([[gnu::unused]] auto i : range::to(n)) {
        if (_queue.empty()) {
          return i;
        }
        _queue.consume_one([&](resumable_builder* r) {
          q->enqueue(r->build());
          own< resumable_builder >{r}.reset();
        });
      }
      return n;
    }

    inline bool shared_queue::is_empty() const {
      return _queue.empty();
    }
  }
}
}
