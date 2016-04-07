INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR} )

add_subdirectory (core)
add_subdirectory (ext)
add_subdirectory (io)
add_subdirectory (hw)
add_subdirectory (util)

add_library(xi ${xi_SRC})
set(COVERAGE_SRCS "${COVERAGE_SRCS}" "${xi_SRC}" CACHE STRING "" FORCE)
if (HWLOC)
  target_link_libraries(xi hwloc)
endif ()

# play test
add_subdirectory (play)
add_subdirectory (tester)
