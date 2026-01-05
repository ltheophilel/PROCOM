#include "../include/moteur.h"
#include "../include/pwm_lookup_table.h"


moteur_config moteur0 = {
    .pin_SA  = 3,
    .pin_SB  = 2,
    .pin_EN  = 1,
    .pin_DIR = 0,
    .last_position = 0,
    .motor_position = 0,
    .motor_speed_rpm = 0.0f
};

moteur_config moteur1 = {
    .pin_SA  = 7,
    .pin_SB  = 6,
    .pin_EN  = 5,
    .pin_DIR = 4,
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

    // gpio_set_irq_enabled_with_callback(
    //     motor->pin_SA, GPIO_IRQ_EDGE_RISE, true, &encoder_isr
    // );
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
    if (level > 100.f) level = 100.f;
    pwm_set_gpio_level(motor->pin_EN, (uint16_t)(level*PWM_WRAP / 100.f));
}

// Ajuste le duty cycle (0 à PWM_WRAP)
void motor_set_pwm_brut(moteur_config *motor, uint16_t level) {
    pwm_set_gpio_level(motor->pin_EN, level);
}

void motor_set_direction(moteur_config *motor, bool direction) {
    gpio_put(motor->pin_DIR, direction);
}


float motor_get_speed(moteur_config *motor) {
    return motor->motor_speed_rpm;
};

// Fast lookup using stored table (no sweep). Returns an estimated PWM level (0..PWM_WRAP).
// This implementation finds the first table index j where speed[j] >= target, and interpolates
// between j-1 and j. This is robust to leading zero values and non-strict monotonicity.
uint32_t pwm_lookup_for_rpm(float target_rpm) {
    // handle trivial bounds
    if (target_rpm <= pwm_table_speed_abs[0]) return 0;
    if (target_rpm >= pwm_table_speed_abs[PWM_TABLE_SIZE-1]) return PWM_WRAP;
    // find first j such that speed[j] >= target_rpm
    for (uint32_t j = 1; j < PWM_TABLE_SIZE; ++j) {
        float b = pwm_table_speed_abs[j];
        if (b >= target_rpm) {
            uint32_t i = j - 1;
            float a = pwm_table_speed_abs[i];
            uint32_t l0 = (i == PWM_TABLE_SIZE-1) ? PWM_WRAP : (uint32_t)(i * 25);
            uint32_t l1 = (j == PWM_TABLE_SIZE-1) ? PWM_WRAP : (uint32_t)(j * 25);
            if (b == a) {
                return l0;
            }
            float ratio = (target_rpm - a) / (b - a);
            float est = (float)l0 + ratio * (float)(l1 - l0);
            if (est < 0.0f) est = 0.0f;
            if (est > (float)PWM_WRAP) est = (float)PWM_WRAP;
            // printf("[Table] Interpolating between idx=%u(level=%u,spd=%.2f) and idx=%u(level=%u,spd=%.2f) -> est=%.1f\n",
            //        i, l0, a, j, l1, b, est);
            return (uint32_t)est;
        }
    }

    // fallback: return last level
    return PWM_WRAP;
}