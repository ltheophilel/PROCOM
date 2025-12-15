add_library(traitement_lib STATIC
    src/traitement.c
)

target_include_directories(traitement_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(traitement_lib
)
