file(GLOB play_SRC "*.cpp")

add_executable(play ${play_SRC})
target_link_libraries(play xi-core xi-async xi-io xi-hw pthread ${Boost_LIBRARIES})

add_executable(playAsio asio.c++)
target_link_libraries(playAsio pthread ${Boost_LIBRARIES})