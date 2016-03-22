#pragma once

#include "xi/ext/configure.h"
#include "xi/io/net/socket_option.h"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

namespace xi {
namespace io {
  namespace net {
    namespace option {
      template < int L, int N, socket_option_access A = READ_WRITE >
      using bool_opt = boolean_socket_option< L, N, A >;
      template < int L, int N, socket_option_access A = READ_WRITE >
      using num_opt = numeric_socket_option< L, N, A >;

      namespace socket {
        using reuse_address = bool_opt< SOL_SOCKET, SO_REUSEADDR >;
        // using reuse_port = bool_opt< SOL_SOCKET, SO_REUSEPORT >;
        using accept_conn = bool_opt< SOL_SOCKET, SO_ACCEPTCONN >;
        using broadcast   = bool_opt< SOL_SOCKET, SO_BROADCAST >;
        using dont_route  = bool_opt< SOL_SOCKET, SO_DONTROUTE >;
        using keep_alive  = bool_opt< SOL_SOCKET, SO_KEEPALIVE >;
        using oob_inline  = bool_opt< SOL_SOCKET, SO_OOBINLINE >;
        using timestamp   = bool_opt< SOL_SOCKET, SO_TIMESTAMP >;

        using priority             = num_opt< SOL_SOCKET, SO_PRIORITY >;
        using receive_buffer       = num_opt< SOL_SOCKET, SO_RCVBUF >;
        using receive_buffer_force = num_opt< SOL_SOCKET, SO_RCVBUFFORCE >;
        using send_buffer          = num_opt< SOL_SOCKET, SO_SNDBUF >;
        using send_buffer_force    = num_opt< SOL_SOCKET, SO_SNDBUFFORCE >;
        using receive_watermark    = num_opt< SOL_SOCKET, SO_RCVLOWAT >;
        using send_watermark = num_opt< SOL_SOCKET, SO_SNDLOWAT, READ_ONLY >;
        using protocol       = num_opt< SOL_SOCKET, SO_PROTOCOL, READ_ONLY >;
        using socket_type    = num_opt< SOL_SOCKET, SO_TYPE, READ_ONLY >;
      }

      namespace ip {
        using free_bind            = bool_opt< IPPROTO_IP, IP_FREEBIND >;
        using mtu_discover         = bool_opt< IPPROTO_IP, IP_MTU_DISCOVER >;
        using allow_multicast_loop = bool_opt< IPPROTO_IP, IP_MULTICAST_LOOP >;
        // using raw_no_defrag = bool_opt< IPPROTO_IP, IP_NODEFRAG >;
        using transparent_proxy = bool_opt< IPPROTO_IP, IP_TRANSPARENT >;

        using mtu           = num_opt< IPPROTO_IP, IP_MTU >;
        using multicast_ttl = num_opt< IPPROTO_IP, IP_MULTICAST_TTL >;
        using ttl           = num_opt< IPPROTO_IP, IP_TTL >;

        struct tos : public socket_option< i32, IPPROTO_IP, IP_TOS > {
          using base = socket_option< i32, IPPROTO_IP, IP_TOS >;
          using base::base;

          static tos low_delay;
          static tos throughput;
          static tos reliability;
          static tos min_cost;
        };
        tos tos::low_delay(IPTOS_LOWDELAY);
        tos tos::throughput(IPTOS_THROUGHPUT);
        tos tos::reliability(IPTOS_RELIABILITY);
        tos tos::min_cost(IPTOS_MINCOST);
      }

      namespace tcp {
        using cork      = bool_opt< IPPROTO_TCP, TCP_CORK >;
        using no_delay  = bool_opt< IPPROTO_TCP, TCP_NODELAY >;
        using quick_ack = bool_opt< IPPROTO_TCP, TCP_QUICKACK >;

        using keep_alive_count    = num_opt< IPPROTO_TCP, TCP_KEEPCNT >;
        using keep_alive_idle     = num_opt< IPPROTO_TCP, TCP_KEEPIDLE >;
        using keep_alive_interval = num_opt< IPPROTO_TCP, TCP_KEEPINTVL >;
        using fin_wait_linger     = num_opt< IPPROTO_TCP, TCP_LINGER2 >;
        using max_segment         = num_opt< IPPROTO_TCP, TCP_MAXSEG >;

        using syn_retransmit_count =
            socket_option< u8, IPPROTO_TCP, TCP_SYNCNT >;
      }

      namespace udp {
        using cork = bool_opt< IPPROTO_UDP, UDP_CORK >;
      }
    }
  }
}
}
