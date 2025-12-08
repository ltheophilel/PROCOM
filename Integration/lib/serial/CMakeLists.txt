add_library(serial_lib STATIC
    src/serial.c
)

target_include_directories(serial_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(serial_lib
    pico_stdlib
    pico_stdio_usb
    pico_cyw43_arch
    pico_cyw43_arch_lwip_threadsafe_background
)
