file(GLOB xi_core_SRC "*.cpp")

add_subdirectory(libevent)

set(xi_SRC ${xi_SRC} ${xi_core_SRC} PARENT_SCOPE)

register_test(future_test xi)
register_test(kernel_test xi)
register_test(latch_test xi)
register_test(task_queue_test xi)
