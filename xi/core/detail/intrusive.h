#pragma once

#include "xi/ext/configure.h"
#include "xi/core/resumable.h"

namespace xi {
namespace core {
  namespace detail {
    using block_queue_type =
      intrusive::list< resumable,
                       intrusive::member_hook< resumable,
                                               block_hook_type,
                                               &resumable::block_hook >,
                       intrusive::constant_time_size< false > >;

    using ready_queue_type =
        intrusive::list< resumable,
                         intrusive::member_hook< resumable,
                                                 ready_hook_type,
                                                 &resumable::ready_hook >,
                         intrusive::constant_time_size< false > >;

    using sleep_queue_type =
        intrusive::set< resumable,
                        intrusive::member_hook< resumable,
                                                sleep_hook_type,
                                                &resumable::sleep_hook >,
                        intrusive::constant_time_size< false >,
                        intrusive::compare< resumable::timepoint_less > >;
  }
}
}
