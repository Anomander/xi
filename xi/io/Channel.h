#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/DataMessage.h"

namespace xi {
namespace io {

  class Channel : virtual public ownership::StdShared {
  protected:
    virtual ~Channel() noexcept = default;

  public:
    virtual void close() = 0;
    virtual void write(own< Message >) = 0;
    virtual size_t read(ByteRange range) = 0;
    virtual size_t read(initializer_list< ByteRange > range) = 0;
    virtual bool isClosed() = 0;
  };
}
}
