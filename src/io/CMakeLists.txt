file(GLOB src "*.cpp")

add_subdirectory(net)
add_subdirectory(pipes)
add_subdirectory(proto)

set(xi_src ${xi_src} ${src} PARENT_SCOPE)

# register_test(tcp_client_socket_integration_test xi)
register_test(fragment_test xi)
register_test(fragment_string_test xi)
register_test(buffer_test xi)
register_test(byte_blob_test xi)
