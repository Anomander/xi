#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace core {
  namespace detail {
    // using block_queue_type =
    //   intrusive::list< resumable,
    //                    intrusive::member_hook< resumable,
    //                                            block_hook_type,
    //                                            &resumable::block_hook >,
    //                    intrusive::constant_time_size< false > >;

    using ready_hook_type = intrusive::list_member_hook<
        intrusive::link_mode< intrusive::auto_unlink > >;

    template < class T >
    using ready_queue_type = intrusive::list<
        T,
        intrusive::member_hook< T, ready_hook_type, &T::ready_hook >,
        intrusive::constant_time_size< false > >;

    using sleep_hook_type = intrusive::set_member_hook<
        intrusive::link_mode< intrusive::auto_unlink > >;

    template < class T >
    using sleep_queue_type = intrusive::set<
        T,
        intrusive::member_hook< T, sleep_hook_type, &T::sleep_hook >,
        intrusive::constant_time_size< false >,
        intrusive::compare< typename T::timepoint_less > >;
  }
}
}
