file(GLOB io_SRC "*.cpp")

add_subdirectory(pipeline)

add_library(xi-io ${io_SRC})
target_link_libraries(xi-io xi-async)

# Temporarily ignoring
# register_test(TcpClientSocketIntegrationTest xi-async xi-io)
