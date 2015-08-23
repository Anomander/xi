file(GLOB hw_SRC "*.cpp")

add_library(xi-hw ${hw_SRC})
target_link_libraries(xi-hw hwloc)
