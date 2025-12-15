#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "lib/Moteur/include/moteur.h"
#include "pico/time.h"

// ----------------- MAIN -----------------

int main() {
    stdio_init_all();
    // Petit délai pour ouvrir le moniteur série
    sleep_ms(2000);

    printf("\n=== TEST MOTEUR + ENCODEUR (10s ON) ===\n");

    // ---------- BROCHES DU MOTEUR ----------
    
    gpio_init(PIN_DIR);
    gpio_set_dir(PIN_DIR, GPIO_OUT);

    motor_pwm_init(PIN_EN);

    // ---------- BROCHES DE L'ENCODEUR ----------
    gpio_init(ENC_SA);
    gpio_set_dir(ENC_SA, GPIO_IN);
    gpio_pull_up(ENC_SA);

    gpio_init(ENC_SB);
    gpio_set_dir(ENC_SB, GPIO_IN);
    gpio_pull_up(ENC_SB);

    // ---------- INTERRUPTION SUR ENC_SA ----------
    gpio_set_irq_enabled_with_callback(
        ENC_SA,
        GPIO_IRQ_EDGE_RISE,
        true,
        &encoder_isr
    );

    // ---------- TIMER POUR LE CALCUL DE LA VITESSE / ANGLE ----------
    repeating_timer_t speed_timer;
    add_repeating_timer_ms(
        -SPEED_PERIOD_MS,   // négatif → répétitif
        speed_timer_cb,
        NULL,
        &speed_timer
    );

    // ---------- ALLUMER LE MOTEUR PENDANT 10s ----------

    // Définit le sens (0 ou 1 selon le câblage)
    gpio_put(PIN_DIR, 0);

    // Duty cycle élevé (comme dans le code qui fonctionnait)
    motor_set_duty(PIN_EN, PWM_DUTY_START);

    printf("Moteur ALLUMÉ (duty élevé) pendant 10 secondes...\n");

    sleep_ms(10000);   // 10 000 ms = 10 s

    // ---------- ÉTEINDRE LE MOTEUR ----------

    motor_set_duty(PIN_EN, 0);
    printf("Moteur ÉTEINT.\n");

    // Continue d'afficher les valeurs finales
    while (true) {
        sleep_ms(500);
    }
}
