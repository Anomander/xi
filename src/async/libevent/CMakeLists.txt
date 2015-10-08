file(GLOB xi_async_libevent_SRC "*.cpp")

set(xi_async_SRC ${xi_async_SRC} ${xi_async_libevent_SRC} PARENT_SCOPE)