file(GLOB xi_io_SRC "*.cpp")

add_subdirectory(net)
add_subdirectory(pipes)
add_subdirectory(proto)

set(xi_SRC ${xi_SRC} ${xi_io_SRC} PARENT_SCOPE)

# register_test(tcp_client_socket_integration_test xi)
register_test(fragment_test xi)
register_test(fragment_string_test xi)
register_test(buffer_test xi)
register_test(byte_blob_test xi)
