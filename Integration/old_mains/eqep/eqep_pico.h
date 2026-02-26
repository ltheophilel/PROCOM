#ifndef EQEP_H
#define EQEP_H

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "quadrature_encoder.pio.h"

#define EQEP_PIO pio0

struct eqep_pico
{
    uint8_t id;
    uint8_t pin;
    uint8_t sm;
    int32_t count;
    int32_t offset;
};

void eqep_pico_init();
void eqep_pico_add(struct eqep_pico * eqep, uint8_t eqep_id, uint8_t eqep_pin);
int32_t eqep_pico_get_count(struct eqep_pico * eqep);
void eqep_pico_set_count(struct eqep_pico * eqep, int32_t val);

#endif