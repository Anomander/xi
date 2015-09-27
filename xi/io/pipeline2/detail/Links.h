#pragma once

#include "xi/ext/Configure.h"
#include "xi/io/pipeline2/Modifiers.h"
#include "xi/io/pipeline2/detail/GenericFilterContext.h"

namespace xi {
namespace io {
  namespace pipe2 {
    namespace detail {

      template < class Message >
      struct ReadLink : virtual GenericFilterContext {
        virtual void read(Message) = 0;
      };

      template < class Message >
      struct WriteLink : virtual GenericFilterContext {
        virtual void write(Message) = 0;
      };
    }
  }
}
}
