#include "xi/core/message_bus.h"
#include "xi/ext/lockfree.h"
#include "xi/core/message_bus_impl.h"
#include "xi/core/shard.h"

namespace xi {
namespace core {

  class message_bus::impl {
    enum {
      QUEUE_CAPACITY = 128,
      QUEUE_BATCH    = 16,
    };
    using message_queue =
        lockfree::spsc_queue< message*, lockfree::capacity< QUEUE_CAPACITY > >;

    struct alignas(64) work_queue {
      deque< message* > backlog;
      message_queue messages;
      usize message_count;
      mut< shard > destination;
    };

    work_queue _pending;
    work_queue _complete;

  public:
    impl(mut< shard > local, mut< shard > remote) {
      _pending.destination  = remote;
      _complete.destination = local;
    }
    void submit(message*);
    usize process_pending();
    usize process_complete();

  private:
    void _reply(message* msg) {
      _complete.backlog.push_back(msg);
      if (_complete.backlog.size() >= QUEUE_BATCH) {
        _flush_backlog(edit(_complete));
      }
    }

    usize _flush_backlog(mut< work_queue > work) {
      auto queue_room = QUEUE_CAPACITY - work->message_count;
      auto move_count = std::min(queue_room, work->backlog.size());
      if (!move_count) {
        return 0;
      }
      auto begin = work->backlog.begin();
      auto end   = begin + move_count;
      work->messages.push(begin, end);
      // work->shard->maybe_wakeup();
      work->backlog.erase(begin, end);
      work->message_count += move_count;
      return move_count;
    }

    template < class F >
    usize _process_queue(mut< message_queue > q, F&& f) {
      message* items[QUEUE_CAPACITY];
      auto popped = _pending.messages.pop(items);
      for (auto i : range::to(popped)) {
        f(items[i]);
      }
      return popped;
    }
  };

  usize message_bus::impl::process_pending() {
    _flush_backlog(edit(_pending));
    return _process_queue(edit(_pending.messages), [this](auto* msg) {
      msg->process_request();
      _reply(msg);
    });
  }

  usize message_bus::impl::process_complete() {
    _flush_backlog(edit(_complete));
    return _process_queue(edit(_complete.messages), [this](auto* msg) {
      msg->process_reply();
      delete msg;
    });
  }

  void message_bus::impl::submit(message* msg) {
    _pending.backlog.push_back(msg);
    if (_pending.backlog.size() >= QUEUE_BATCH) {
      _flush_backlog(edit(_pending));
    }
  }

  message_bus::message_bus(mut< shard > local, mut< shard > remote)
      : _impl(make_unique< impl >(local, remote)) {
  }

  usize message_bus::process_pending() {
    return _impl->process_pending();
  }

  usize message_bus::process_complete() {
    return _impl->process_complete();
  }

  void message_bus::_submit(unique_ptr< message > msg) {
    _impl->submit(msg.release());
  }
}
}
