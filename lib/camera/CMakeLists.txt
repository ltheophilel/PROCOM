add_library(camera_lib STATIC
    src/ov7670.c
    src/camera.c
    src/init_camera.c
)

target_include_directories(camera_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(camera_lib 
    pico_stdlib 
    hardware_i2c
)
