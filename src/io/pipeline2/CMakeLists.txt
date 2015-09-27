file(GLOB io_pipeline2_SRC "*.cpp")

set(io_SRC ${io_SRC} ${io_pipeline2_SRC} PARENT_SCOPE)

register_test(Pipeline2Test xi-io)