file(GLOB async_SRC "*.cpp")

add_subdirectory(libevent)

add_library(xi-async ${async_SRC})
target_link_libraries(xi-async xi-core event)

register_test(FutureTest xi-async)
register_test(LatchTest xi-async)
