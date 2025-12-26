add_library(framebuffer INTERFACE)

target_sources(framebuffer INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/color.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/framebuffer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/st7796.cpp
)

target_include_directories(framebuffer INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(framebuffer INTERFACE
    pico_stdlib
    hardware_dma
    hardware_spi
    misc
)