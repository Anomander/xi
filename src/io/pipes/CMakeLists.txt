file(GLOB xi_io_pipes_SRC "*.cpp")

add_subdirectory(detail)

set(xi_io_SRC ${xi_io_SRC} ${xi_io_pipes_SRC} PARENT_SCOPE)

register_test(pipes_test xi)
