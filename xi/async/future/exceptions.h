#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace async {

  struct broken_promise_exception : public std::logic_error {
    broken_promise_exception()
        : std::logic_error("This promise has been broken") {}
  };

  struct invalid_promise_exception : public std::logic_error {
    invalid_promise_exception() : std::logic_error("This promise is invalid") {}
  };
}
}
