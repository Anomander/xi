file(GLOB play_SRC "*.cpp")

add_executable(play ${play_SRC})
target_link_libraries(play xi-async xi-io pthread ${Boost_LIBRARIES})
