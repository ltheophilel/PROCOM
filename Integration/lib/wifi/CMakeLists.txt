# Crée une bibliothèque statique pour les sources wifi
add_library(wifi_lib STATIC
    src/tcp_server.c
    src/wifi_connect.c
)

# Inclure les répertoires nécessaires
target_include_directories(wifi_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# Lier les bibliothèques nécessaires
target_link_libraries(wifi_lib
    pico_stdlib
    pico_cyw43_arch
    pico_cyw43_arch_lwip_threadsafe_background
)

# Augmenter la taille de la heap si nécessaire
set(PICO_STACK_SIZE 2048)
set(PICO_HEAP_SIZE 8192)