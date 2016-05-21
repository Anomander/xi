#include "xi/core/resumable.h"
#include "xi/core/abstract_worker.h"

namespace xi {
namespace core {

  void resumable::attach_executor(abstract_worker* e) {
    _worker = e;
  }

  void resumable::detach_executor(abstract_worker* e) {
    assert(_worker == e);
    _worker = nullptr;
  }

  void resumable::unblock() {
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

  void resumable::sleep_for(milliseconds ms) {
    assert(_worker != nullptr);
    _worker->sleep_for(this, ms);
    block();
  }

}
}
