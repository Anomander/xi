#pragma once

#include "xi/ext/Configure.h"

namespace xi {
namespace io {
  namespace pipe2 {

    template < class Message >
    struct ReadOnly {};
    template < class Message >
    struct WriteOnly {};

    template < class Message >
    using In = ReadOnly< Message >;
    template < class Message >
    using Out = WriteOnly< Message >;
    template < class Message >
    using ReadWrite = Message;
    template < class Message >
    using InOut = Message;

  }
}
}
