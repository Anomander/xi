#pragma once

#include "xi/io/socket_option.h"

#include <netinet/in.h>

namespace xi {
namespace io {

  using reuse_address = boolean_socket_option< SOL_SOCKET, SO_REUSEADDR >;
}
}
