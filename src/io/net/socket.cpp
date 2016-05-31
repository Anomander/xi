#include "xi/io/net/socket.h"
#include "xi/core/all.h"
#include "xi/io/error.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

namespace xi {
namespace io {
  namespace net {
    int socket::read_some(char* data, size_t size) {
      int r = -1;
      for (;;) {
        r = ::recv(fd(), data, size, MSG_DONTWAIT);
        // printf("%s %d\n", __PRETTY_FUNCTION__, r);
        if (r < 0) {
          if (errno == EINTR) {
            yield();
            continue;
          }
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            await_readable();
            continue;
          }
          return -1;
        }
        if (r == 0) {
          return {xi::io::error::kEOF};
        }
        break;
      }
      return r;
    }

    int socket::write_some(char* data, size_t size) {
      int r = -1;
      for (;;) {
        // printf("%s\n", __PRETTY_FUNCTION__);
        r = ::send(fd(), data, size, MSG_DONTWAIT | MSG_NOSIGNAL);
        if (r < 0) {
          if (errno == EINTR) {
            continue;
          }
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            await_writable();
            continue;
          }
          return -1;
        }
        break;
      }
      return r;
    }

    size_t socket::write_exactly(char* data, size_t size) {
      auto* p = data;
      auto s  = 0ull;
      while (s < size) {
        auto ret = write_some(p, size - s);
        if (ret < 0) {
          return ret;
        }
        auto r = ret;
        p += r;
        s += r;
        // yield();
      }
      return s;
    }
  };
}
}
