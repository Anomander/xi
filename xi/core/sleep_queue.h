#pragma once

#include "xi/ext/configure.h"
#include "xi/core/detail/intrusive.h"
#include "xi/core/resumable.h"
#include "xi/core/resumable_builder.h"
#include "xi/core/worker_queue.h"
#include "xi/util/spin_lock.h"

namespace xi {
namespace core {
  namespace v2 {

    class local_sleep_queue : public ownership::unique {
      detail::sleep_queue_type< resumable > _queue;
      steady_clock::time_point _next_item = steady_clock::time_point::max();

    public:
      void enqueue(steady_clock::time_point when, own< resumable >);
      void dequeue_into(mut< worker_queue >, steady_clock::time_point cutoff);
      bool is_empty() const;
      steady_clock::time_point next_item() const;
    };

    inline void local_sleep_queue::enqueue(steady_clock::time_point when,
                                           own< resumable > r) {
      assert(is_valid(r));
      if (_next_item > when) {
        _next_item = when;
      }
      r->wakeup_time(when);
      _queue.insert(*(r.release()));
    }

    inline void local_sleep_queue::dequeue_into(
        mut< worker_queue > q, steady_clock::time_point cutoff) {
      if (_next_item > cutoff) {
        return;
      }

      auto i   = _queue.begin();
      auto end = _queue.end();
      while (i != end) {
        if (i->wakeup_time() > cutoff) {
          _next_item = i->wakeup_time();
          return;
        }
        i->wakeup_time(steady_clock::time_point::max());
        q->enqueue(own< resumable >{&(*i)});
        i = _queue.erase(i);
      }
      // reached the end of the queue
      _next_item = steady_clock::time_point::max();
    }

    inline bool local_sleep_queue::is_empty() const {
      return _queue.empty();
    }

    inline steady_clock::time_point local_sleep_queue::next_item() const {
      return _next_item;
    }

    class shared_sleep_queue : public ownership::unique {
      local_sleep_queue _queue;
      spin_lock _lock;

    public:
      void enqueue(steady_clock::time_point, own< resumable >);
      void dequeue_into(mut< worker_queue >, steady_clock::time_point cutoff);
      bool is_empty() const;
      steady_clock::time_point next_item() const;
    };

    inline void shared_sleep_queue::enqueue(steady_clock::time_point when,
                                            own< resumable > r) {
      assert(is_valid(r));
      auto lock = make_unique_lock(_lock);
      _queue.enqueue(when, move(r));
    }

    inline void shared_sleep_queue::dequeue_into(
        mut< worker_queue > q, steady_clock::time_point cutoff) {
      auto lock = make_unique_lock(_lock);
      _queue.dequeue_into(q, cutoff);
    }

    inline bool shared_sleep_queue::is_empty() const {
      return _queue.is_empty();
    }

    inline steady_clock::time_point shared_sleep_queue::next_item() const {
      return _queue.next_item();
    }
  }
}
}
