#include "xi/ext/configure.h"
#include "xi/io/net/channel_options.h"

namespace xi {
namespace io {
  namespace net {
    namespace option {

      namespace ip {
        tos tos::low_delay(IPTOS_LOWDELAY);
        tos tos::throughput(IPTOS_THROUGHPUT);
        tos tos::reliability(IPTOS_RELIABILITY);
        tos tos::min_cost(IPTOS_MINCOST);
      }
    }
  }
}
}
