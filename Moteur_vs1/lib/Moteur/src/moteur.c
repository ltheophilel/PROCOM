#include "../include/moteur.h"


// ----------------- VARIABLES GLOBALES -----------------
// 'volatile' car modifiées dans l'ISR

volatile int32_t motor_position   = 0;   // position en "comptes"
volatile int32_t last_position    = 0;   // position lors du dernier calcul
volatile float   motor_speed_rpm  = 0.0f;


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

    // ----- NOUVEAU : calcul de l’angle -----
    float rev     = COUNTS_TO_REV(pos);
    float angle_d = REV_TO_DEG(rev);

    printf("Counts = %ld  |  Angle = %.2f deg  |  Vitesse = %.2f RPM\n",
           (long)pos, angle_d, motor_speed_rpm);

    return true; // continue d’exécuter le timer
}


// ----------------- FONCTIONS UTILITAIRES PWM -----------------

// Configure le PWM sur la broche EN sans activer le moteur
void motor_pwm_init(int gpio_en) {
    gpio_set_function(gpio_en, GPIO_FUNC_PWM);

    int slice_num = pwm_gpio_to_slice_num(gpio_en);

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
void motor_set_duty(int gpio_en, int16_t level) {
    if (level > PWM_WRAP) level = PWM_WRAP;
    if (level < 0)        level = 0;
    pwm_set_gpio_level(gpio_en, level);
}


// ----------------- FONCTIONS D’ACCÈS (GETTERS) -----------------

int32_t motor_get_position_counts(void) {
    return motor_position;
}

float motor_get_position_rev(void) {
    return COUNTS_TO_REV(motor_position);
}

float motor_get_angle_deg(void) {
    float rev = COUNTS_TO_REV(motor_position);
    return REV_TO_DEG(rev);
}

float motor_get_angle_rad(void) {
    float rev = COUNTS_TO_REV(motor_position);
    return REV_TO_RAD(rev);
}

float motor_get_speed_rpm(void) {
    return motor_speed_rpm;
}

float motor_get_speed_rad_s(void) {
    // RPM → rad/s = RPM * (2*pi / 60)
    return motor_speed_rpm * (6.28318530718f / 60.0f);
}

/*
// À activer si tu définis WHEEL_CIRCUM_MM dans moteur.h
float motor_get_linear_speed_mm_s(void) {
    // vitesse (rev/s) = RPM / 60
    float rev_s = motor_speed_rpm / 60.0f;
    return rev_s * WHEEL_CIRCUM_MM;
}
*/
