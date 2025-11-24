# lib/CMakeLists.txt
add_library(led STATIC
    led/src/led.c
)
target_include_directories(led PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/led/include
)

add_library(wifi STATIC
    wifi/src/tcp_server.c
    wifi/src/wifi_connect.c
)
target_include_directories(wifi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/wifi/include
)

add_library(oled STATIC
    oled/src/digitRenderer.c
    oled/src/i2cSend.c
)
target_include_directories(oled PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/oled/include
)
