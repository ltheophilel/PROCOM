#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "lib/lib.h"

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif


// Traite une commande reçue
// appelé dans handle_serial_input
void process_command(const char *command) {
    if (strcasecmp(command, "LED ON") == 0) {
        pico_set_led(true);
        printf("✅ LED allumée\r\n");
    } else if (strcasecmp(command, "LED OFF") == 0) {
        pico_set_led(false);
        printf("💤 LED éteinte\r\n");
    } else {
        printf("Commande inconnue: %s\r\n", command);
    }
}



int main() {
    stdio_init_all();                // initialise STDIO (USB)
    pico_led_init();           // initialise la LED
    float v_mot = 0;

    char line[LINE_BUF_SIZE];
    size_t idx = 0;

    // Petit message d'accueil
    printf("Pico C - Serial command interface ready\r\n");
    uint64_t t_us_previous = time_us_64();
    while (true) {
        v_mot++;
        update_motor_value(&t_us_previous, 1000000, 0, &v_mot);
        handle_serial_input(line, &idx, process_command);
    }
    return 0;
}
