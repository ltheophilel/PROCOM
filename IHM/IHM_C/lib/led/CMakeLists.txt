add_library(led_lib STATIC
    src/led.c
)

target_include_directories(led_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(led_lib
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
)
