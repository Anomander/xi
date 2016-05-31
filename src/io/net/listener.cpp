#include "xi/io/net/listener.h"
#include "xi/core/all.h"
#include "xi/io/error.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

namespace xi {
namespace io {
  namespace net {
    listener::listener(u16 p)
        : port(::socket(AF_INET,
                        SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP)) {
      struct sockaddr_in addr;
      ::memset(&addr, 0, sizeof(addr));
      addr.sin_family      = AF_INET;
      addr.sin_port        = htons(p);
      addr.sin_addr.s_addr = INADDR_ANY;
      int reuse            = 1;
      setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
      if (bind(fd(), (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
      }
      if (::listen(fd(), 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
      }
    }

    result< i32 > listener::accept() {
      sockaddr_in* addr;
      socklen_t len = sizeof(addr);
      for (;;) {
        int sock = ::accept4(
            fd(), (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (sock < 0) {
          if (errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            await_readable();
            continue;
          }
          perror("accept");
          return result_from_errno< i32 >();
        }
        return ok(sock);
      }
    }

    core::channel< i32 > listener::as_channel() {
      core::channel< i32 > ch = core::make_channel< i32 >();
      spawn([this, ch] {
        for (;;) {
          auto r = accept().then([ch = move(ch)](i32 sock) mutable {
            ch.send(sock);
          });
          if (r.is_error()) {
            break;
          }
        }
      });
      return ch;
    }
  }
}
}
