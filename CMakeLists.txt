cmake_minimum_required (VERSION 2.8.0 FATAL_ERROR)

project(Xi)

message("Compiler version: " ${CMAKE_CXX_COMPILER_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -O3 -g -ftemplate-backtrace-limit=0")
# set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -O3 -save-temps=obj -g -fsanitize=undefined -ftemplate-backtrace-limit=0")

# boost
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_NO_BOOST_CMAKE         ON)
set(Boost_DETAILED_FAILURE_MSG   ON)

# boost
find_package( Boost 1.55 REQUIRED thread system context )

# libevent
# find_package( Event )
add_subdirectory(third-party/libevent)
INCLUDE_DIRECTORIES( ${libevent_SOURCE_DIR}/include )
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR}/third-party/libevent/include )

# jemalloc
# Will revisit when working with memory
#find_package( JeMalloc )

# googletest
add_subdirectory(third-party/googletest)
enable_testing()
INCLUDE_DIRECTORIES( ${gtest_SOURCE_DIR}/include )

# register_test(name)
#
# creates a named C++ test that depends on common
# libraries with the default compiler flags.
function(register_test name)
  cxx_executable_with_flags(${name} "${cxx_default}" "" "test/${name}.c++")
  # set(libraries ${Boost_LIBRARIES})
  foreach(arg ${ARGN})
    set(libraries ${libraries} ${arg})
  endforeach()
  target_link_libraries(${name} ${libraries} ${Boost_LIBRARIES} gtest_main)
  add_test("${name}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}")
endfunction()

if (NOT DEFINED XI_CMAKE_OUTPUT_DIR)
  set(XI_CMAKE_OUTPUT_DIR ${CMAKE_BINARY_DIR})
endif (NOT DEFINED XI_CMAKE_OUTPUT_DIR)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${XI_CMAKE_OUTPUT_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${XI_CMAKE_OUTPUT_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${XI_CMAKE_OUTPUT_DIR}/lib )

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

if (DEFINED XI_CMAKE_HAS_HWLOC)
  add_definitions(-DXI_HAS_HWLOC)
endif (DEFINED XI_CMAKE_HAS_HWLOC)

add_subdirectory(src)
