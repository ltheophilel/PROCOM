#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lib.h"
#include "hardware/i2c.h"

#define I2C_SDA 8
#define I2C_SCL 9


// Supprime les espaces en début et fin de chaîne
char* trim_whitespace(char *line) {
    char *s = line;
    while (*s && (*s == ' ' || *s == '\t')) s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\0' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    return s;
}

int estNombreEntier(const char *s) {
    char *end;
    long val = strtol(s, &end, 10);
    // Vérifie si la conversion a consommé toute la chaîne et qu'il n'y a pas eu d'erreur
    return (*end == '\0' && s != end);
}

void interpretCommand(TCP_SERVER_T *state, const char* command) {
    // Implémentez ici l'interprétation des commandes reçues
    command = trim_whitespace((char *)command);
    printf("Interpreted Command: '%s'\n", command);
    if (strcmp(command, "LED ON") == 0) {
        pico_set_led(true);
        tcp_server_send(state, "LED is ON");
    } else if (strcmp(command, "LED OFF") == 0) {
        pico_set_led(false);
        tcp_server_send(state, "LED is OFF");
    } else if (estNombreEntier(command)) {
        const char *prefix = "M0 :";
        char result[100]; // Buffer statique
        snprintf(result, sizeof(result), "%s%s", prefix, command);
        tcp_server_send(state, result);
    } else {
        // Réponse simple : renvoyer exactement ce qu'on a reçu
        tcp_server_send(state, command);
    }
}

int main() {
    stdio_init_all();
    // sleep_ms(10000);

    // err_t wifi_fail = connect_to_wifi(WIFI_SSID, WIFI_PASSWORD, 10000);
    // if (!wifi_fail) {
    //     pico_set_led(true); // on success
    // }
    wifi_auto_connect();
    pico_set_led(true); 
    
    TCP_SERVER_T* state = tcp_server_start();
    // IP4ADDR
    // uint64_t t_us_previous = time_us_64();
    // uint64_t t_us_current = time_us_64();


    initOLED(I2C_SDA, I2C_SCL);
    renderSymbol(RPI,1,56); // Render the Rpi Symbol
    renderIPString(IP4ADDR);

    uint8_t rx_buffer[BUF_SIZE];
    while (true) {
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
        #endif
        int received = tcp_server_receive(state, rx_buffer, BUF_SIZE);

        if (received > 0) {
            // Ajouter un '\0' pour créer une chaîne C
            rx_buffer[received] = '\0';

            printf("Reçu du client : %s\n", rx_buffer);

            // Réponse simple : renvoyer exactement ce qu'on a reçu
            interpretCommand(state, (const char *)rx_buffer);
        }
        sleep_ms(100);
        
        // uint64_t t_us_current = time_us_64();
        // if (t_us_current - t_us_previous >= 1000000) { // 1 second
        //     t_us_previous = t_us_current;
        //     pico_toggle_led();
        //     // printf("hello\n");
        //     tcp_server_send(state, (const uint8_t *)"Hello, client!", 14);
        // }
    }

    // tcp_server_stop(state);
    pico_set_led(false);
    cyw43_arch_deinit();
    return 0;
}



