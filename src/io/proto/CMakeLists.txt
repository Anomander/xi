file(GLOB xi_io_proto_SRC "*.cpp")

add_subdirectory(http)
add_subdirectory(http2)

set(xi_io_SRC ${xi_io_SRC} ${xi_io_proto_SRC} PARENT_SCOPE)
