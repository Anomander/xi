file(GLOB async_SRC "*.cpp")

add_subdirectory(libevent)

add_library(xi-async ${async_SRC})
target_link_libraries(xi-async event hwloc)

register_test(FutureTest)
register_test(LatchTest)
