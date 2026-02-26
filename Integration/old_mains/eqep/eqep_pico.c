#include "eqep_pico.h"

void eqep_pico_init()
{
    uint offset_enc;

    pio_clear_instruction_memory(EQEP_PIO);
    pio_add_program_at_offset(EQEP_PIO, &quadrature_encoder_program,0);	

}

void eqep_pico_add(struct eqep_pico * eqep, uint8_t eqep_id, uint8_t eqep_pin)
{
    eqep->sm = pio_claim_unused_sm(EQEP_PIO,0);	
    eqep->pin = eqep_pin;
    quadrature_encoder_program_init(EQEP_PIO, eqep->sm, 0, eqep->pin, 0);
    sleep_ms(100);
    eqep->offset = 0;//quadrature_encoder_get_count(EQEP_PIO,eqep->sm);
    eqep->count = 0;
}

int32_t eqep_pico_get_count(struct eqep_pico * eqep)
{
    eqep->count = quadrature_encoder_get_count(EQEP_PIO, eqep->sm) - eqep->offset;
    return eqep->count;
}

void eqep_pico_set_count(struct eqep_pico * eqep, int32_t val)
{
    eqep->offset = val;// (eqep->count - val);
}
