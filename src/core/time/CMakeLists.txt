file(GLOB src "*.cpp")
set(xi_src ${xi_src} ${src} PARENT_SCOPE)

register_test(time_benchmark)
