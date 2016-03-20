file(GLOB xi_io_proto_http2_SRC "*.cpp")

set(xi_io_proto_SRC ${xi_io_proto_SRC} ${xi_io_proto_http2_SRC} PARENT_SCOPE)

register_test(http2_decoder_test xi)
