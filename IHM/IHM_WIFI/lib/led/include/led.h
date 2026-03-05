/* led.h - simple LED management for Pico / Pico W
 * Provides pico_led_init() and pico_set_led()
 */
#ifndef LED_H
#define LED_H

#include <stdbool.h>

#include "pico/stdlib.h"
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/* Initialise the LED hardware.
 * Returns 0 on success or non-zero on failure.
 */
int pico_led_init(void);

/* Set LED state: true = on, false = off */
void pico_set_led(bool led_on);

void pico_toggle_led(void);
void pico_get_led_state(bool *state);
void pico_blink_led(int times, int delay_ms);

#endif /* LED_H */



// struct moteur_config {
//     uint8_t SA_pin; // ou unsigned int si tu préfères
//     uint8_t SB_pin;
// };

// struct moteur_config moteur1 = { .SA_pin = 2, .SB_pin = 3 };