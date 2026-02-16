#ifndef MOTEUR_H
#define MOTEUR_H

#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"


// --------------- Struture du moteur -------------

typedef struct {
    int pin_SA;   //
    int pin_SB;   //
    int pin_EN;   // Broche PWM
    int pin_DIR;  // Broche de direction
    int32_t motor_position;   // position en "comptes"
    int32_t last_position;   // position lors du dernier calcul
    float motor_speed_rpm;  // vitesse en RPM
} moteur_config;

extern moteur_config moteur0;
extern moteur_config moteur1;


// ----------------- PARAMÈTRES DU PWM -----------------

// ~20 kHz : 125 MHz / (6249 + 1) ≈ 20 kHz
#define PWM_WRAP        6249
// Duty élevé pour garantir que le moteur tourne (≈ 100%)
#define PWM_DUTY_START  6248    

// ----------------- PARAMÈTRES DE L'ENCODEUR / VITESSE -----------------

#define SPEED_PERIOD_MS     1000        // calcul de la vitesse toutes les 1 s
#define ENC_COUNTS_PER_REV  318.0f      // TODO : remplacer par la valeur réelle de l’encodeur

// ----------------- VARIABLES GLOBALES -----------------

#define V_ROTATION 10  // Vitesse de rotation en RPM lors de la recherche de ligne
enum etat_recherche_ligne {
    RECULER = 0,
    TOURNER_DROITE = 1,
    TOURNER_GAUCHE = 2
};

// ----------------- PROTOTYPES -----------------

void init_motor_and_encoder(moteur_config *motor);
void init_all_motors_and_encoders();
void set_motor_encoder(moteur_config *motor, uint speed_period_ms);
void motor_set_pwm(moteur_config *motor, float level);
void motor_set_pwm_brut(moteur_config *motor, uint16_t level);
void motor_set_rpm(moteur_config *motor, float target_rpm);
void motor_set_direction(moteur_config *motor, bool direction);
void motor_define_direction_from_pwm(int v_mot_droit, int v_mot_gauche);
float motor_get_speed(moteur_config *motor);
uint32_t pwm_lookup_for_rpm(float target_rpm);
void chercher_ligne(int v_droit, int v_gauche, double angle, short pourcentage_V_marche_arriere);

#endif