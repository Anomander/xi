#pragma once

#include "xi/ext/configure.h"
#include "xi/io/basic_buffer_allocator.h"

namespace xi {
namespace io {

  class channel_interface {
  public:
    virtual ~channel_interface()              = default;
    virtual mut< fragment_allocator > alloc() = 0;
    virtual void close()                      = 0;
  };
}
}
