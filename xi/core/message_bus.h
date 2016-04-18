#pragma once

#include "xi/ext/configure.h"
#include "xi/core/future.h"

namespace xi {
namespace core {

  class shard;

  class message_bus {
    class impl;
    unique_ptr< impl > _impl;

  public:
    struct message;

    message_bus(mut< shard > local, mut< shard > remote);

    template < class F >
    future< future_result< F > > submit(F&& f);

    usize process_pending();
    usize process_complete();

  public:
    void _submit(unique_ptr< message >);
  };
}
}
