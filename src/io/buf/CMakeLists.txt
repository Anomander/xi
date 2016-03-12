file(GLOB xi_io_buf_SRC "*.cpp")

set(xi_io_SRC ${xi_io_SRC} ${xi_io_buf_SRC} PARENT_SCOPE)

register_test(buffer_test xi)
register_test(buf_test xi)
register_test(chain_test xi)
