
add_compile_options(-Wall -Wextra -Werror)

# ws35_test

add_executable(ws35_test
    ws35_test.cpp
)

pico_enable_stdio_uart(ws35_test 0)
pico_enable_stdio_usb(ws35_test 1)

target_link_libraries(ws35_test PRIVATE
    pico_rand
    pico_stdlib
    pico_stdio_usb
    hardware_spi
    framebuffer
    misc
)

pico_add_extra_outputs(ws35_test)


# ws24_test

add_executable(ws24_test
    ws24_test.cpp
)

pico_enable_stdio_uart(ws24_test 0)
pico_enable_stdio_usb(ws24_test 1)

target_link_libraries(ws24_test PRIVATE
    pico_rand
    pico_stdlib
    pico_stdio_usb
    hardware_spi
    framebuffer
    misc
)

pico_add_extra_outputs(ws24_test)
