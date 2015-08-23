file(GLOB core_SRC "*.cpp")

add_library(xi-core ${core_SRC})
target_link_libraries(xi-core xi-hw)

register_test(TaskQueueTest)
register_test(KernelTest xi-core)
