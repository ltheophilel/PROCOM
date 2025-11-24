#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"

// Pico W devices use a GPIO on the WIFI chip for the LED,
// so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif


#define LINE_BUF_SIZE 128


// Perform initialisation
int pico_led_init(void) {
    stdio_init_all();
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}


void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}


int main() {
    stdio_init_all();                // initialise STDIO (UART et USB CDC si activé)
    pico_led_init();           // initialise la LED
    int v_mot = 0;

    char line[LINE_BUF_SIZE];
    size_t idx = 0;

    // Petit message d'accueil
    printf("Pico C - Serial command interface ready\r\n");
    uint64_t t_us_previous = time_us_64();
    uint64_t t_us_current = time_us_64();
    while (true) {
        uint64_t t_us_current = time_us_64();
        if (t_us_current - t_us_previous >= 1000000) { // 1 second
            t_us_previous = t_us_current;
            v_mot += 1;
            printf("M: %d\r\n", v_mot);
        }

        // int c = getchar(); // blocant : attend un caractère sur stdio (USB CDC si activé)
        // printf("Caractère reçu: %c (code %d)\r\n", c, c);
        
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0); // Lecture non-bloquante
            if (c != PICO_ERROR_TIMEOUT) {
        
                if (c == EOF) {
                    // si aucune entrée, on peut faire un yield léger
                    sleep_ms(1);
                    continue;
                }

                if (c == '\r') {
                    // ignore CR, attend LF ou considerer fin ici
                    continue;
                }

                if (c == '\n' || idx >= LINE_BUF_SIZE - 1) {
                    // fin de ligne : terminer la chaîne et traiter la commande
                    line[idx] = '\0';
                    if (idx > 0) {
                        // Trim éventuels espaces en début/fin
                        // Trim left
                        char *s = line;
                        while (*s && (*s == ' ' || *s == '\t')) s++;
                        // Trim right
                        char *end = s + strlen(s) - 1;
                        while (end >= s && (*end == ' ' || *end == '\t')) {
                            *end = '\0';
                            end--;
                        }

                        if (strcasecmp(s, "LED ON") == 0) {
                            pico_set_led(true);
                            printf("✅ LED allumée\r\n");
                        } else if (strcasecmp(s, "LED OFF") == 0) {
                            pico_set_led(false);
                            printf("💤 LED éteinte\r\n");
                        } else {
                            printf("Commande inconnue: %s\r\n", s);
                        }
                    }
                    // reset buffer
                    idx = 0;
                    memset(line, 0, LINE_BUF_SIZE);
                } else {
                    // ajouter caractère dans buffer
                    line[idx++] = (char)c;
                }
            }
        }

    }
    return 0;
}
