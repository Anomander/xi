file(GLOB xi_core_SRC "*.cpp")

set(xi_SRC ${xi_SRC} ${xi_core_SRC} PARENT_SCOPE)

#add_library(xi-core ${core_SRC})
#target_link_libraries(xi-core xi-hw)

register_test(task_queue_test)
register_test(kernel_test xi)
