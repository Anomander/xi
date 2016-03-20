file(GLOB xi_io_net_SRC "*.cpp")

set(xi_io_SRC ${xi_io_SRC} ${xi_io_net_SRC} PARENT_SCOPE)

register_test(socket_test xi)
register_test(udp_socket_test xi)
register_test(stream_server_socket_test xi)
