#include "async/WakeUpEvent.h"

#include <sys/eventfd.h>

namespace xi {
namespace async {

WakeUpEvent::WakeUpEvent() : IoHandler(eventfd(0, 0)) {}

void WakeUpEvent::fire() noexcept {
  auto result = eventfd_write(descriptor(), 1);
  (void)result;
}

void WakeUpEvent::handleRead() {
  eventfd_t result;
  eventfd_read(descriptor(), &result);
}
}
}
