file(GLOB xi_io_proto_http_SRC "*.cpp")

set(xi_io_proto_SRC ${xi_io_proto_SRC} ${xi_io_proto_http_SRC} PARENT_SCOPE)

register_test(http_decoder_test xi)
