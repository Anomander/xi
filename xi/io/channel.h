#pragma once

#include "xi/ext/configure.h"
#include "xi/io/data_message.h"

namespace xi {
namespace io {

  class channel : virtual public ownership::std_shared {
  protected:
    virtual ~channel() noexcept = default;

  public:
    virtual void close() = 0;
    virtual void write(own< message >) = 0;
    virtual void write(byte_range) = 0;
    virtual size_t read(byte_range range) = 0;
    virtual size_t read(initializer_list< byte_range > range) = 0;
    virtual bool is_closed() = 0;
  };
}
}
