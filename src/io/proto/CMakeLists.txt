file(GLOB src "*.cpp")

add_subdirectory(http)
add_subdirectory(http2)

set(xi_src ${xi_src} ${src} PARENT_SCOPE)
