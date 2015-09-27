file(GLOB io_SRC "*.cpp")

add_subdirectory(pipeline)
add_subdirectory(pipeline2)

add_library(xi-io ${io_SRC})
target_link_libraries(xi-io xi-async)

register_test(tcp_client_socket_integration_test xi-async xi-io)
register_test(stream_buffer_test)
