#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace async {

  struct BrokenPromiseException : public std::logic_error {
    BrokenPromiseException() : std::logic_error("This promise has been broken") {}
  };

  struct InvalidPromiseException : public std::logic_error {
    InvalidPromiseException() : std::logic_error("This promise is invalid") {}
  };
}
}
