
#include "../include/multicore.h"

// Buffers pour les images et synchronisation multicœur
static uint8_t* outbuf1;
static uint8_t* outbuf2;
static bool toggle_buf = false; // Alternance des buffers entre cœurs


// échange les buffers d'image entre les deux cœurs   
uint8_t** get_outbuf_from_core(bool core) {
    uint8_t** outbuf = (toggle_buf == core) ? &outbuf1 : &outbuf2;
    return outbuf;
}
// gestion de la synchronisation entre les deux cœurs
static bool core_0_ready = false;
static bool core_1_ready = false;
void core_ready_to_swap(bool core, bool ready) {
    switch (core) {
        case 0:
            core_0_ready = ready;
            break;
        case 1:
            core_1_ready = ready;
            break;
    }
    if (core_0_ready && core_1_ready)
    {
        printf("Core 0 using buffer %d, Core 1 using buffer %d\n", toggle_buf ? 0 : 1, toggle_buf ? 1 : 0);
        toggle_buf = !toggle_buf;
        core_0_ready = false;
        core_1_ready = false;
    }
}
