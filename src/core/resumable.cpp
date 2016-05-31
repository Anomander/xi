#include "xi/core/resumable.h"
#include "xi/core/abstract_worker.h"

namespace xi {
namespace core {
  namespace v2 {
    thread_local resumable_stat RESUMABLE_STAT;
  }

  void resumable::attach_executor(abstract_worker* e) {
    _worker = e;
  }

  void resumable::detach_executor(abstract_worker* e) {
    assert(_worker == e);
    _worker = nullptr;
  }

  void resumable::unblock() {
    sleep_hook.unlink();
    ready_hook.unlink();
    _worker->schedule(this);
  }

  void resumable::block() {
    yield(blocked);
  }

  steady_clock::time_point resumable::wakeup_time() {
    return _wakeup_time;
  }

  void resumable::wakeup_time(steady_clock::time_point when) {
    _wakeup_time = when;
  }

  void resumable::sleep_for(nanoseconds ns) {
    assert(_worker != nullptr);
    ready_hook.unlink();
    _worker->sleep_for(this, ns);
    block();
  }

}
}
