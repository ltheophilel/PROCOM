#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PIN_DIR 14
#define PIN_EN  15


#define ENC_SA  16    // canal A do encoder
#define ENC_SB  17    // canal B do encoder


int main() {
    stdio_init_all();

    // Configura pino DIR como saída
    gpio_init(PIN_DIR);
    gpio_set_dir(PIN_DIR, GPIO_OUT);

    // Configura pino EN como PWM
    gpio_set_function(PIN_EN, GPIO_FUNC_PWM);

    // Pega o slice (unidade PWM) associado ao pino EN
    uint slice_num = pwm_gpio_to_slice_num(PIN_EN);

    // Define frequência próxima a 20 kHz
    // Fórmula: freq = clock / (wrap + 1)
    // 125 MHz / (6249 + 1) = 20 kHz
    pwm_set_wrap(slice_num, 6249);

    // Define duty cycle (0–6249)
    pwm_set_gpio_level(PIN_EN, 6248);  // 50%

    // Ativa o PWM
    pwm_set_enabled(slice_num, true);

    // Define direção horário
    gpio_put(PIN_DIR, 1);

    // Roda o motor por 3 segundos
    sleep_ms(3000);

    // Para o motor
    pwm_set_gpio_level(PIN_EN, 0);

    while (1) {
        tight_loop_contents();
    }
}