#pragma once

#include "xi/ext/configure.h"

namespace xi {
namespace io {
  class port {
    i32 _fd;

  protected:
    port(i32 fd);
    port(port&& other);
    ~port();
    XI_CLASS_DEFAULTS(port, no_copy);

    i32 fd() const;
    void await_readable();
    void await_writable();
  };

  inline i32 port::fd() const {
    return _fd;
  }

  class pollable_fd {
    i32 _fd;

  protected:
    pollable_fd(i32 fd);
    pollable_fd(pollable_fd&& other);
    ~pollable_fd();
    XI_CLASS_DEFAULTS(pollable_fd, no_copy);

    i32 fd() const;
    void await_readable();
    void await_writable();
  };

  inline i32 pollable_fd::fd() const {
    return _fd;
  }
}
}
