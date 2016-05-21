INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR} )

add_custom_target(xi_ragel)
add_subdirectory (core)
add_subdirectory (ext)
add_subdirectory (io)
add_subdirectory (hw)
add_subdirectory (util)

add_library(xi SHARED ${xi_src})

add_dependencies(xi xi_ragel)
set(COVERAGE_SRCS "${COVERAGE_SRCS}" "${xi_src}" CACHE STRING "" FORCE)
if (HWLOC)
  target_link_libraries(xi hwloc)
endif ()
target_link_libraries(xi rt)

# play test
add_subdirectory (play)
add_subdirectory (tester)
