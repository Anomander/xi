file(GLOB src "*.cpp")

add_subdirectory(detail)

set(xi_src ${xi_src} ${src} PARENT_SCOPE)

register_test(pipes_test xi)
