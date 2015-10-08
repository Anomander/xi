file(GLOB xi_hw_SRC "*.cpp")

set(xi_SRC ${xi_SRC} ${xi_hw_SRC} PARENT_SCOPE)

#add_library(xi-hw ${hw_SRC})
#set(xi_hw_LIBRARIES)

#if (DEFINED XI_CMAKE_HAS_HWLOC)
#set(xi_hw_LIBRARIES ${xi_hw_LIBRARIES} hwloc)
#endif (DEFINED XI_CMAKE_HAS_HWLOC)

#target_link_libraries(xi-hw ${xi_hw_LIBRARIES})
