file(GLOB io_pipeline_SRC "*.cpp")

set(io_SRC ${io_SRC} ${io_pipeline_SRC} PARENT_SCOPE)

register_test(PipelineTest xi-io)