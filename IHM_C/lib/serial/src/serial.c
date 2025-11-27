

#include "../include/serial.h"


// Supprime les espaces en début et fin de chaîne
char* trim_whitespace(char *line) {
    char *s = line;
    while (*s && (*s == ' ' || *s == '\t')) s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return s;
}


// Gère la lecture série et le traitement des commandes
void handle_serial_input(char *line, size_t *idx, void (*command_callback)(const char *)) {
    if (stdio_usb_connected()) {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == EOF) {
                sleep_ms(1);
                return;
            }
            if (c == '\r') {
                return; // Ignore CR
            }
            if (c == '\n' || *idx >= LINE_BUF_SIZE - 1) {
                line[*idx] = '\0';
                if (*idx > 0) {
                    char *s = trim_whitespace(line);
                    command_callback(s); // Appelle la fonction de rappel
                }
                *idx = 0;
                memset(line, 0, LINE_BUF_SIZE);
            } else {
                line[(*idx)++] = (char)c;
            }
        }
    }
}


// Met à jour la valeur du moteur toutes les secondes
void update_motor_value(uint64_t *t_us_previous, uint32_t t_us_between, bool which_mot, float *v_mot) {
    uint64_t t_us_current = time_us_64();
    if (t_us_current - *t_us_previous >= t_us_between) { // 1 seconde
        *t_us_previous = t_us_current;
        printf("M%d: %f\r\n", which_mot, *v_mot);
    }
}