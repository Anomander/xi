file(GLOB tester_SRC "*.cpp")

add_executable(tester ${tester_SRC})
target_link_libraries(tester xi-async ${Boost_LIBRARIES})
