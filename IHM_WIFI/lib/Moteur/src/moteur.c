#include "../include/moteur.h"


moteur_config moteur1 = {
    .pin_SA  = 16,
    .pin_SB  = 17,
    .pin_EN  = 15,
    .pin_DIR = 14,
    .last_position = 0,
    .motor_position = 0,
    .motor_speed_rpm = 0.0f
};

moteur_config moteur2 = {
    .pin_SA  = 16,
    .pin_SB  = 17,
    .pin_EN  = 15,
    .pin_DIR = 14,
    .last_position = 0,
    .motor_position = 0,
    .motor_speed_rpm = 0.0f
};

#define MAX_MOTEURS 2

static moteur_config* moteurs_actifs[MAX_MOTEURS];
static int moteurs_count = 0;




// ----------------- ISR DE L'ENCODEUR -----------------
// Appelée à chaque front montant sur pin_SA

void encoder_isr(uint gpio, uint32_t events) {
    for (int i = 0; i < moteurs_count; i++) {
        moteur_config *m = moteurs_actifs[i];

        if (gpio == m->pin_SA && (events & GPIO_IRQ_EDGE_RISE)) {
            bool sb = gpio_get(m->pin_SB);

            if (!sb)
                m->motor_position++;
            else
                m->motor_position--;
        }
    }
}


// Configure le PWM sur la broche EN sans activer le moteur
void motor_pwm_init(moteur_config *motor) {
    gpio_set_function(motor->pin_EN, GPIO_FUNC_PWM);

    int slice_num = pwm_gpio_to_slice_num(motor->pin_EN);

    // Désactive le slice avant configuration
    pwm_set_enabled(slice_num, false);

    // Configure la fréquence (~20 kHz)
    pwm_set_wrap(slice_num, PWM_WRAP);

    // Niveau initial à 0 (moteur arrêté)
    pwm_set_gpio_level(motor->pin_EN, 0);

    // Active le PWM
    pwm_set_enabled(slice_num, true);
}


void init_motor_and_encoder(moteur_config *motor) {

    // Enregistrer dans la liste des moteurs actifs
    moteurs_actifs[moteurs_count++] = motor;

    // Init matériel (GPIO, PWM, etc.)
    gpio_init(motor->pin_DIR);
    gpio_set_dir(motor->pin_DIR, GPIO_OUT);

    motor_pwm_init(motor);

    gpio_init(motor->pin_SA);
    gpio_set_dir(motor->pin_SA, GPIO_IN);
    gpio_pull_up(motor->pin_SA);

    gpio_init(motor->pin_SB);
    gpio_set_dir(motor->pin_SB, GPIO_IN);
    gpio_pull_up(motor->pin_SB);

    gpio_set_irq_enabled_with_callback(
        motor->pin_SA, GPIO_IRQ_EDGE_RISE, true, &encoder_isr
    );
}


// ----------------- TIMER POUR LE CALCUL DE LA VITESSE -----------------
// Exécuté toutes les SPEED_PERIOD_MS millisecondes

bool speed_timer_cb(repeating_timer_t *t) {
    moteur_config *motor = (moteur_config *)t->user_data;

    int32_t pos   = motor->motor_position;
    int32_t delta = pos - motor->last_position;
    motor->last_position = pos;

    float period_s = SPEED_PERIOD_MS / 1000.0f;

    // comptes/seconde → RPM
    // Vérifier !!! ! ! ! !! ! ! ! !! ! ! ! ! !     
    motor->motor_speed_rpm = (delta / period_s) / ENC_COUNTS_PER_REV * 60.0f;

    printf("Position = %ld  |  Vitesse = %.2f RPM\n",
           (long)pos, motor->motor_speed_rpm);

    return true; // continue d’exécuter le timer
}


 // ---------- TIMER POUR LE CALCUL DE LA VITESSE ----------
void set_motor_encoder(moteur_config *motor, uint speed_period_ms)
{
    repeating_timer_t speed_timer;
    add_repeating_timer_ms(
        -speed_period_ms,   // négatif → répétitif
        speed_timer_cb,
        motor,
        &speed_timer
    );
}






// Ajuste le duty cycle (0 à PWM_WRAP)
void motor_set_pwm(moteur_config *motor, float level) {
    if (level > 100.) level = 100.;
    pwm_set_gpio_level(motor->pin_EN, level*PWM_WRAP);
}

void motor_set_direction(moteur_config *motor, bool direction) {
    gpio_put(motor->pin_DIR, direction);
}
