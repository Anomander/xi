cmake_minimum_required (VERSION 2.8)

project(Xi)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -O3 -g -ftemplate-backtrace-limit=0")
# set(CMAKE_CXX_FLAGS "-std=c++1y -Wall -O3 -save-temps=obj -g -fsanitize=undefined -ftemplate-backtrace-limit=0")

# boost
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_NO_BOOST_CMAKE         ON)
set(Boost_DETAILED_FAILURE_MSG   ON)

find_package( Boost 1.55 REQUIRED COMPONENTS system timer thread )

# libevent
find_package( Event )

# jemalloc
find_package( JeMalloc )

# googletest
add_subdirectory(third-party/googletest)
enable_testing()
INCLUDE_DIRECTORIES( ${gtest_SOURCE_DIR}/include )

# register_test(name)
#
# creates a named C++ test that depends on common
# libraries with the default compiler flags.
function(register_test name)
  cxx_executable_with_flags(${name} "${cxx_default}" gtest_main "test/${name}.c++")
  set(libraries ${Boost_LIBRARIES})
  foreach(arg ${ARGN})
    set(libraries ${libraries} ${arg})
  endforeach()
  target_link_libraries(${name} ${libraries})
  add_test("${name}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}")
endfunction()

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/src )

add_subdirectory(src)
