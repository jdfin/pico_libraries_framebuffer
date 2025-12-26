
add_compile_options(-Wall -Wextra -Werror)

# st7796_test

add_executable(st7796_test
    st7796_test.cpp
)

pico_enable_stdio_uart(st7796_test 0)
pico_enable_stdio_usb(st7796_test 1)

target_link_libraries(st7796_test PRIVATE
    pico_stdlib
    pico_stdio_usb
    hardware_spi
    framebuffer
    misc
)

pico_add_extra_outputs(st7796_test)
