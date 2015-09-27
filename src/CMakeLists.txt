INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR} )

add_subdirectory (core)
add_subdirectory (async)
add_subdirectory (ext)
add_subdirectory (io)
add_subdirectory (hw)
add_subdirectory (util)

# play test
add_subdirectory (play)
add_subdirectory (tester)
