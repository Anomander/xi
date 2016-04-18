file(GLOB src "*.cpp")
set(xi_src ${xi_src} ${src} PARENT_SCOPE)

register_test(http2_decoder_test xi)
