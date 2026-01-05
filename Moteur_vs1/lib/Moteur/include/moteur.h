#ifndef MOTEUR_H
#define MOTEUR_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"


// --------------- Structure du moteur -------------

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
// Duty élevé pour garantir que le moteur tourne
#define PWM_DUTY_START  3124  // ≈ 50%    


// ----------------- PARAMÈTRES DE L'ENCODEUR / VITESSE -----------------

#define SPEED_PERIOD_MS     1000        // calcul de la vitesse toutes les 1 s

// Nombre de comptes de l’encodeur par tour mécanique
// (tu as mis 318.0f car c’est ce qui donnait la bonne vitesse)
#define ENC_COUNTS_PER_REV  318.0f      

// Conversion pour l’angle
#define COUNTS_TO_REV(counts)   ((float)(counts) / ENC_COUNTS_PER_REV)
#define REV_TO_DEG(rev)         ((rev) * 360.0f)
#define REV_TO_RAD(rev)         ((rev) * 6.28318530718f)   // 2 * pi

// (OPTIONNEL) Si tu connais la circonférence de ta roue en mm:
// #define WHEEL_CIRCUM_MM       100.0f   // TODO: remplace par la vraie valeur
// #define COUNTS_TO_MM(counts)  (COUNTS_TO_REV(counts) * WHEEL_CIRCUM_MM)


// ----------------- PROTOTYPES -----------------

// Interruptions et timer
void encoder_isr(uint gpio, uint32_t events);
bool speed_timer_cb(repeating_timer_t *t);

// PWM
void motor_pwm_init(int gpio_en);
void motor_set_duty(int gpio_en, int16_t level);

// Fonctions d’accès aux grandeurs calculées
int32_t motor_get_position_counts(void);
float   motor_get_position_rev(void);
float   motor_get_angle_deg(void);
float   motor_get_angle_rad(void);
float   motor_get_speed_rpm(void);
float   motor_get_speed_rad_s(void);
// float   motor_get_linear_speed_mm_s(void); // à activer si tu définis WHEEL_CIRCUM_MM

#endif
