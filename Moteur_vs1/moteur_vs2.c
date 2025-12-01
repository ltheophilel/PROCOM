#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

// ----------------- DÉFINITION DES BROCHES -----------------

// Connexion au driver du moteur (ex : PmodHB5)
#define PIN_DIR 14      // Broche de direction (DIR)
#define PIN_EN  15      // PWM du moteur (EN / ENA)

// Broches de l’encodeur du moteur
#define ENC_SA  16      // Canal A (génère une interruption)
#define ENC_SB  17      // Canal B (indique le sens)

// ----------------- PARAMÈTRES DU PWM -----------------

// ~20 kHz : 125 MHz / (6249 + 1) ≈ 20 kHz
#define PWM_WRAP        6249
// Duty élevé pour garantir que le moteur tourne (≈ 100%)
#define PWM_DUTY_START  6248    

// ----------------- PARAMÈTRES DE L'ENCODEUR / VITESSE -----------------

#define SPEED_PERIOD_MS     1000        // calcul de la vitesse toutes les 1 s
#define ENC_COUNTS_PER_REV  100.0f      // TODO : remplacer par la valeur réelle de l’encodeur

// ----------------- VARIABLES GLOBALES -----------------

volatile int32_t motor_position   = 0;   // position en "comptes"
volatile int32_t last_position    = 0;   // position lors du dernier calcul
volatile float   motor_speed_rpm  = 0.0f;

// ----------------- PROTOTYPES -----------------

void encoder_isr(uint gpio, uint32_t events);
bool speed_timer_cb(repeating_timer_t *t);
void motor_pwm_init(uint gpio_en);
void motor_set_duty(uint gpio_en, uint16_t level);

// ----------------- ISR DE L'ENCODEUR -----------------
// Appelée à chaque front montant sur ENC_SA

void encoder_isr(uint gpio, uint32_t events) {
    if (gpio == ENC_SA && (events & GPIO_IRQ_EDGE_RISE)) {
        bool sb = gpio_get(ENC_SB);  // lit le canal B

        if (!sb) {
            motor_position++;        // un sens de rotation
        } else {
            motor_position--;        // sens opposé
        }
    }
}

// ----------------- TIMER POUR LE CALCUL DE LA VITESSE -----------------
// Exécuté toutes les SPEED_PERIOD_MS millisecondes

bool speed_timer_cb(repeating_timer_t *t) {
    (void)t; // évite un avertissement

    int32_t pos   = motor_position;
    int32_t delta = pos - last_position;
    last_position = pos;

    float period_s = SPEED_PERIOD_MS / 1000.0f;

    // comptes/seconde → RPM
    motor_speed_rpm = (delta / period_s) / ENC_COUNTS_PER_REV * 60.0f;

    printf("Position = %ld  |  Vitesse = %.2f RPM\n",
           (long)pos, motor_speed_rpm);

    return true; // continue d’exécuter le timer
}

// ----------------- FONCTIONS UTILITAIRES -----------------

// Configure le PWM sur la broche EN sans activer le moteur
void motor_pwm_init(uint gpio_en) {
    gpio_set_function(gpio_en, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio_en);

    // Désactive le slice avant configuration
    pwm_set_enabled(slice_num, false);

    // Configure la fréquence (~20 kHz)
    pwm_set_wrap(slice_num, PWM_WRAP);

    // Niveau initial à 0 (moteur arrêté)
    pwm_set_gpio_level(gpio_en, 0);

    // Active le PWM
    pwm_set_enabled(slice_num, true);
}

// Ajuste le duty cycle (0 à PWM_WRAP)
void motor_set_duty(uint gpio_en, uint16_t level) {
    if (level > PWM_WRAP) level = PWM_WRAP;
    pwm_set_gpio_level(gpio_en, level);
}

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

    // ---------- TIMER POUR LE CALCUL DE LA VITESSE ----------
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

    // Continue d'afficher les valeurs finales (position/vitesse = 0)
    while (true) {
        sleep_ms(500);
    }
}
