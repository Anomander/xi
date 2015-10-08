file(GLOB xi_async_SRC "*.cpp")

add_subdirectory(libevent)

set(xi_SRC ${xi_SRC} ${xi_async_SRC} PARENT_SCOPE)

register_test(future_test xi)
register_test(latch_test xi)
