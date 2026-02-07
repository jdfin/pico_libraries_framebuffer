add_library(framebuffer INTERFACE)

target_sources(framebuffer INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/framebuffer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/tft.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ws24.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ws35.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/hy35.cpp
)

target_include_directories(framebuffer INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/fonts
)

target_link_libraries(framebuffer INTERFACE
    pico_stdlib
    hardware_dma
    hardware_spi
    misc
)

add_subdirectory(test)
