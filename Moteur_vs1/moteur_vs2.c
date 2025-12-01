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

    init_motor_and_encoder(&moteur1);

    set_motor_encoder(&moteur1, 1000);

    // ---------- ALLUMER LE MOTEUR PENDANT 10s ----------

    // Définit le sens (0 ou 1 selon le câblage)
    motor_set_direction(&moteur1, 0);

    // Duty cycle élevé (comme dans le code qui fonctionnait)
    motor_set_pwm(&moteur1, 100.);

    printf("Moteur ALLUMÉ (duty élevé) pendant 10 secondes...\n");

    sleep_ms(10000);   // 10 000 ms = 10 s

    // ---------- ÉTEINDRE LE MOTEUR ----------

    motor_set_pwm(&moteur1, 0.);
    printf("Moteur ÉTEINT.\n");

    // Continue d'afficher les valeurs finales (position/vitesse = 0)
    while (true) {
        sleep_ms(500);
    }
}
