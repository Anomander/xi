#pragma once

#include "xi/ext/configure.h"
#include "xi/io/buffer_allocator.h"

namespace xi {
namespace io {

  class channel_interface {
  public:
    virtual ~channel_interface()            = default;
    virtual mut< buffer_allocator > alloc() = 0;
    virtual void close()                    = 0;
  };
}
}
