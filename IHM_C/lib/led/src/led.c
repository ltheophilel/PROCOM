/* led.c - simple LED management for Pico / Pico W
 */

#include "../include/led.h"

// Variable globale pour stocker l'état de la LED
static bool led_state = false; // false = éteinte, true = allumée

// Perform initialisation
int pico_led_init(void) {
    stdio_init_all();
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return 0;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#else
    return 0; // - no LED to manage
#endif
};

// Turn the led on or off
void pico_set_led(bool led_on) {
    led_state = led_on;
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
};

void pico_toggle_led(void) {
    led_state = !led_state;
    pico_set_led(led_state);
}

void pico_get_led_state(bool *state) {
    if (state) {
        *state = led_state;
    }
}

void pico_blink_led(int times, int delay_ms) {
    for (int i = 0; i < times; i++) {
        pico_set_led(true);
        sleep_ms(delay_ms);
        pico_set_led(false);
        sleep_ms(delay_ms);
    }
}