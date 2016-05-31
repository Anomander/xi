#include "xi/io/pollable_fd.h"
#include "xi/core/all.h"

namespace xi {
namespace io {
  port::port(i32 fd) : _fd(fd) {
  }

  port::port(port&& other) : _fd(other._fd) {
    other._fd = -1;
  }

  port::~port() {
    if (-1 != _fd) {
      close(_fd);
    }
  }

  void port::await_readable() {
    xi::core::v2::runtime.await_readable(_fd);
  }

  void port::await_writable() {
    xi::core::v2::runtime.await_writable(_fd);
  }

  pollable_fd::pollable_fd(i32 fd) : _fd(fd) {
  }

  pollable_fd::pollable_fd(pollable_fd&& other) : _fd(other._fd) {
    other._fd = -1;
  }

  pollable_fd::~pollable_fd() {
    if (-1 != _fd) {
      close(_fd);
    }
  }

  void pollable_fd::await_readable() {
    xi::core::runtime.local_worker().reactor().await_readable(_fd);
  }

  void pollable_fd::await_writable() {
    xi::core::runtime.local_worker().reactor().await_writable(_fd);
  }
}
}
