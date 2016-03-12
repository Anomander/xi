#pragma once

#include "xi/ext/configure.h"
#include "xi/io/buf/buf.h"

namespace xi {
namespace io {

  class channel_interface {
  public:
    virtual ~channel_interface() = default;
    virtual void close() = 0;
  };

}
}
