#pragma once

#include "xi/ext/configure.h"
#include "xi/core/bootstrap.h"
#include "xi/core/shard.h"

namespace xi {

class server_bootstrap : public core::bootstrap {
  using super = core::bootstrap;
private:
  server_bootstrap() = default;

public:
  static server_bootstrap& create() {
    static server_bootstrap instance;
    return instance;
  }

public:
  template < class Backend >
  server_bootstrap& configure(int argc, char* argv[]) {
    super::configure< Backend >(argc, argv);
    return *this;
  }

  server_bootstrap& before_shutdown(function< void() >&& f) {
    super::before_shutdown(move(f));
    return *this;
  }

  template < class Acceptor, class Endpoint >
  server_bootstrap& bind(Endpoint endpoint,
                         function< void(Acceptor*) >&& initializer) {
    static_assert(true, ""); // TODO: Replace with concept check
    auto acceptor = make< Acceptor >();
    initializer(edit(acceptor));
    acceptor->bind(move(endpoint));
    // TODO: Randomize
    shard()->reactor()->attach_handler(move(acceptor));
    return *this;
  }
};
}
