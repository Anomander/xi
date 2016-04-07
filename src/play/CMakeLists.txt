add_executable(play main.cpp)
target_link_libraries(play xi event pthread ${Boost_LIBRARIES} ${LIBNUMA_LIBRARIES})

# add_executable(context context.cpp)
# target_link_libraries(context pthread)
