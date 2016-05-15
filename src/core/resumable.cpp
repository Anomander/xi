#include "xi/core/resumable.h"
#include "xi/core/execution_context.h"

namespace xi {
namespace core {

  void resumable::attach_executor(execution_context* e) {
    _executor = e;
  }

  void resumable::detach_executor(execution_context* e) {
    assert(_executor == e);
    _executor = nullptr;
  }

  void resumable::unblock() {
    _executor->schedule(this);
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
    assert(_executor != nullptr);
    _executor->sleep_for(this, ms);
    block();
  }

}
}
