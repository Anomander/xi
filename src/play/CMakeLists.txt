add_executable(play main.cpp)
target_link_libraries(play xi-core xi-async xi-hw pthread ${Boost_LIBRARIES})

add_executable(play_asio asio.c++)
target_link_libraries(play_asio pthread ${Boost_LIBRARIES})

add_executable(context context.cpp)
target_link_libraries(context pthread ${Boost_LIBRARIES})