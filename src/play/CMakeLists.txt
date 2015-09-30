file(GLOB play_SRC "*.cpp")

add_executable(play ${play_SRC})
target_link_libraries(play xi-core xi-async xi-hw pthread ${Boost_LIBRARIES})

add_executable(play_asio asio.c++)
target_link_libraries(play_asio pthread ${Boost_LIBRARIES})