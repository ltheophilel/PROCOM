
#ifndef MULTICORE_H
#define MULTICORE_H

#include <stdio.h>
#include "pico/stdlib.h"
#include <stdint.h>

// échange les buffers d'image entre les deux cœurs   
uint8_t** get_outbuf_from_core(bool core);

// préviens que le cœur est prêt ou non pour l'échange de buffers
void core_ready_to_swap(bool core, bool ready);

#endif /* MULTICORE_H */


