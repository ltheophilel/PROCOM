add_library(divers_lib STATIC
    src/divers.c
)

target_include_directories(divers_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(divers_lib
    pico_stdlib
)
