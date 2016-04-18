file(GLOB src "*.cpp")
set(xi_src ${xi_src} ${src} PARENT_SCOPE)

register_test(socket_test xi)
register_test(udp_socket_test xi)
register_test(stream_server_socket_test xi)
