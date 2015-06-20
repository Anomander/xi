#pragma once

#include "ext/Configure.h"
#include "io/Message.h"

namespace xi {
  namespace io {

    class Channel
      : virtual public ownership::StdShared
    {
    protected:
      virtual ~Channel() noexcept = default;
    public:
      virtual void close() = 0;
      virtual void write(own<Message>) = 0;
    };

  }
}
