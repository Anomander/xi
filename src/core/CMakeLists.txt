file(GLOB core_SRC "*.cpp")

add_library(xi-core ${core_SRC})
target_link_libraries(xi-core xi-hw)

register_test(task_queue_test)
register_test(kernel_test xi-core)
