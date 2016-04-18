file(GLOB tester_SRC "*.cpp")

add_executable(tester ${tester_SRC})
target_link_libraries(tester ${XI_EXTERN_LIBRARIES})
