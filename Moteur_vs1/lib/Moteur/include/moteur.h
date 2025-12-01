#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"


// --------------- Struture du moteur -------------

struct moteur_config {
    int pin_SA;   //
    int pin_SB;   //
    int pin_EN;   // Broche PWM
    int pin_DIR;  // Broche de direction
};


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


// ----------------- PROTOTYPES -----------------

void encoder_isr(uint gpio, uint32_t events);
bool speed_timer_cb(repeating_timer_t *t);
void motor_pwm_init(int gpio_en);
void motor_set_duty(int gpio_en, int16_t level);
