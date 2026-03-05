

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"


// Définition des broches GPIO
#define DIR_PIN 14      // Broche pour contrôler la direction (DIR)
#define PWM_PIN 15     // Broche PWM pour contrôler la vitesse (PWM)

// Paramètres PWM
#define PWM_FREQUENCY 20000  // Fréquence PWM en Hz (20 kHz)
#define PWM_WRAP 12500       // Valeur de wrap pour 20 kHz à 125 MHz

void init_pins(void);
void set_direction(bool direction);
void set_speed_percent(int speed_percent);
void stop_motor(void);