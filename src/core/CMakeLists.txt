file(GLOB src "*.cpp")

add_subdirectory(epoll)
# add_subdirectory(libevent)
add_subdirectory(memory)
add_subdirectory(reactor)
add_subdirectory(time)

set(xi_src ${xi_src} ${src} PARENT_SCOPE)

register_test(future_test xi)
# register_test(kernel_test xi)
register_test(latch_test xi)
register_test(task_queue_test xi)
