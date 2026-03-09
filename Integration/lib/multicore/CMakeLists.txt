add_library(multicore_lib STATIC
    src/multicore.c
)

target_include_directories(multicore_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(multicore_lib
                    pico_stdlib)
