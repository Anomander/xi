#include "xi/core/execution_context.h"

namespace xi {
  namespace core {
    // static thread_local execution_context* COORDINATOR;
    alignas(64) thread_local execution_context* WORKER;
  }
}
