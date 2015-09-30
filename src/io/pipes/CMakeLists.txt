file(GLOB io_pipes_SRC "*.cpp")

set(io_SRC ${io_SRC} ${io_pipes_SRC} PARENT_SCOPE)

register_test(pipes_test)