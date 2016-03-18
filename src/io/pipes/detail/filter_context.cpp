#include "xi/ext/configure.h"
#include "xi/io/pipes/detail/filter_context.h"
#include "xi/io/pipes/detail/pipe_control_interface.h"

namespace xi {
namespace io {
  namespace pipes {
    namespace detail {

      void filter_context_base::remove() {
        _pipe->remove(this);
      }

      void filter_context_base::set_pipe(mut< pipe_control_interface > p) {
        _pipe = p;
      }

      mut< pipe_control_interface > filter_context_base::pipe() {
        return _pipe;
      }
    }
  }
}
}
