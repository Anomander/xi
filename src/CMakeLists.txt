INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR} )

add_subdirectory (core)
add_subdirectory (async)
add_subdirectory (ext)
add_subdirectory (io)
add_subdirectory (hw)
add_subdirectory (util)

message(${xi_SRC})
add_library(xi ${xi_SRC})
target_link_libraries(xi)

# play test
add_subdirectory (play)
add_subdirectory (tester)
