cmake_minimum_required (VERSION 2.8.0 FATAL_ERROR)

project(Xi)

message("Compiler version: " ${CMAKE_CXX_COMPILER_VERSION})

option(COVERALLS "Generate coveralls data" OFF)
option(HWLOC "Has hwloc available" OFF)
option(NUMA "Has NUMA support available" OFF)
option(EMULATE_MADVISE "Emulate support for ::madvise flags" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -O3 -g -Wno-overloaded-virtual -Wno-attributes -ftemplate-backtrace-limit=0")
# set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -O0 -g -Wno-overloaded-virtual -Wno-attributes -ftemplate-backtrace-limit=0")

# include (Ragel)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# boost
set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      OFF)
set(Boost_NO_BOOST_CMAKE         ON)
set(Boost_DETAILED_FAILURE_MSG   ON)
set(BOOST_ALL_DYN_LINK           ON)

set(XI_EXTERN_LIBRARIES pthread)

if (NUMA)
  add_definitions(-DXI_HAS_NUMA)
  find_package( LibNuma REQUIRED )
  set(XI_EXTERN_LIBRARIES ${XI_EXTERN_LIBRARIES} ${LIBNUMA_LIBRARIES})
endif()

if (EMULATE_MADVISE)
  add_definitions(-DXI_EMULATE_MADVISE)
endif()

find_package( Boost 1.55 REQUIRED context coroutine thread system program_options)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
set(XI_EXTERN_LIBRARIES ${XI_EXTERN_LIBRARIES} ${Boost_LIBRARIES})

# googletest
add_subdirectory(third-party/googletest)
enable_testing()
INCLUDE_DIRECTORIES( ${gtest_SOURCE_DIR}/include )

if (COVERALLS)
    include(Coveralls)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
endif()

# register_test(name)
#
# creates a named C++ test that depends on common
# libraries with the default compiler flags.
function(register_test name)
  cxx_executable_with_flags(
    ${name}
    # "${cxx_default} -g -O0 -ftest-coverage -fprofile-arcs -fsanitize=address -fsanitize=undefined -fsanitize=leak"
    "${cxx_default} -g -O0 -ftest-coverage -fprofile-arcs"
    # "-lgcov --coverage -lasan -lubsan"
    "-lgcov --coverage"
    "test/${name}.c++"
  )

  foreach(arg ${ARGN})
    set(libraries ${libraries} ${arg})
  endforeach()
  target_link_libraries(${name} ${libraries} ${XI_EXTERN_LIBRARIES} gtest_main)
  add_test("${name}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}")
endfunction()

if (NOT DEFINED XI_CMAKE_OUTPUT_DIR)
  set(XI_CMAKE_OUTPUT_DIR ${CMAKE_BINARY_DIR})
endif (NOT DEFINED XI_CMAKE_OUTPUT_DIR)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${XI_CMAKE_OUTPUT_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${XI_CMAKE_OUTPUT_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${XI_CMAKE_OUTPUT_DIR}/lib )

if (HWLOC)
  add_definitions(-DXI_HAS_HWLOC)
endif ()

add_subdirectory(src)

if (COVERALLS)
  coveralls_turn_on_coverage()
  # Create the coveralls target.
  coveralls_setup(
    "${COVERAGE_SRCS}" # The source files.
    ON                 # If we should upload.
  )
endif()
