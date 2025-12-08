add_library(moteur_lib STATIC
    src/moteur.c)

target_include_directories(moteur_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(moteur_lib
    pico_stdlib
    hardware_pwm
    hardware_gpio
)