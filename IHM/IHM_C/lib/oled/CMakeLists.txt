add_library(oled_lib STATIC
    src/i2cSend.c
    src/digitRenderer.c
)

target_include_directories(oled_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(oled_lib
    pico_stdlib
    hardware_i2c
)