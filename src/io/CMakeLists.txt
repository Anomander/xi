file(GLOB xi_io_SRC "*.cpp")

add_subdirectory(buf)
add_subdirectory(pipes)

set(xi_SRC ${xi_SRC} ${xi_io_SRC} PARENT_SCOPE)

#add_library(xi-io ${io_SRC})
#target_link_libraries(xi-io xi-async)

# register_test(tcp_client_socket_integration_test xi)
register_test(stream_buffer_test)
