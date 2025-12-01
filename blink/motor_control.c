/*
 * motor_control.c
 * Contrôle de Moteur DC avec Pmod HB5 sur Raspberry Pi Pico 2
 *
 * - PWM pour la vitesse (0..100%)
 * - GPIO pour la direction (avant/arrière)
 * - Fréquence PWM ~20 kHz
 *
 * Wiring:
 * - Pmod HB5 IN1 -> PWM_PIN
 * - Pmod HB5 DIR  -> DIR_PIN
 * - GND -> GND
 * - Vmotor -> alimentation séparée selon moteur
 */

 






//#include <stdio.h>
//#include <stdbool.h>
//#include "hardware/pwm.h"
//#include "hardware/gpio.h"
//#include "motor_control.h"

 //int slice_num;

 //void init_pins(void) {
    // Direction pin
  //  gpio_init(DIR_PIN);
  //  gpio_set_dir(DIR_PIN, GPIO_OUT);
  //  gpio_put(DIR_PIN, 0);

    // PWM pin -> configure function
   // gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
   // slice_num = pwm_gpio_to_slice_num(PWM_PIN);

  //  pwm_config config = pwm_get_default_config();
    // We'll set clkdiv to get desired frequency; assumed system clock default 125 MHz
   // pwm_config_set_wrap(&config, PWM_WRAP);
    // choose clock divider 10 -> 125MHz/10/(PWM_WRAP+1) ~= 10000000/(12501) ~ 800 Hz (but this depends)
    // The build below uses the same constants as prior examples; you can tune clkdiv if needed.
  //  pwm_config_set_clkdiv(&config, 10.0f);
  //  pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
  //  pwm_init(slice_num, &config, true);
  //  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PWM_PIN), 0);
//}

// Set direction: true = forward, false = reverse
// void set_direction(bool forward) {
 //   gpio_put(DIR_PIN, forward ? 1 : 0);
//}

// speed: 0..100
// void set_speed_percent(int speed_percent) {
  //  if (speed_percent > 100) speed_percent = 100;
 //   int level = (int)speed_percent * (int)PWM_WRAP / 100u;
 //   pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PWM_PIN), level);
//}

// void stop_motor(void) {
  //  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PWM_PIN), 0);
//}













// int main(void) {
//     stdio_init_all();
//     sleep_ms(500);
//     printf("Motor control (Pmod HB5) - Pico 2\n");

//     init_pins();
//     printf("Pins initialisés. DIR=%d PWM=%d\n", DIR_PIN, PWM_PIN);

//     // Demo sequence
//     while (true) {
//         printf("Forward 50%%\n");
//         set_direction(true);
//         set_speed_percent(50);
//         sleep_ms(3000);

//         printf("Stop\n");
//         stop_motor();
//         sleep_ms(1000);

//         printf("Reverse 30%%\n");
//         set_direction(false);
//         set_speed_percent(30);
//         sleep_ms(3000);

//         printf("Stop\n");
//         stop_motor();
//         sleep_ms(1000);

//         printf("Forward ramp up\n");
//         set_direction(true);
//         for (int i = 0; i <= 100; i += 10) {
//             set_speed_percent(i);
//             sleep_ms(200);
//         }
//         sleep_ms(800);

//         printf("Forward ramp down\n");
//         for (int i = 100; i >= 0; i -= 10) {
//             set_speed_percent((int)i);
//             sleep_ms(200);
//         }
//         stop_motor();
//         sleep_ms(2000);
//     }

//     return 0;
// }


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

// ----------------- DÉFINITION DES BROCCHES -----------------

// Connexion avec le driver (PmodHB5, par exemple)
#define PIN_DIR 14      // DIR du driver
#define PIN_EN  15      // EN / ENA du driver

// Encodeur du moteur
#define ENC_SA  16      // canal A (génère une interruption)
#define ENC_SB  17      // canal B (sens)

// ----------------- PARAMÈTRES PWM -----------------

// ~20 kHz : 125 MHz / (6249 + 1) ≈ 20 kHz
#define PWM_WRAP        6249
// Duty cycle élevé pour garantir que le moteur tourne (≈ 100%)
#define PWM_DUTY_START  6248    

// ----------------- PARAMÈTRES ENCODEUR / VITESSE -----------------

#define SPEED_PERIOD_MS     1000        // calcul de vitesse toutes les 1 s
#define ENC_COUNTS_PER_REV  100.0f      // TODO : remplacer par la valeur réelle de l'encodeur

// ----------------- VARIABLES GLOBALES -----------------

volatile int32_t motor_position   = 0;   // position en comptages
volatile int32_t last_position    = 0;   // position à la dernière mesure
volatile float   motor_speed_rpm  = 0.0f;

// ----------------- PROTOTYPES -----------------

void encoder_isr(uint gpio, uint32_t events);
bool speed_timer_cb(repeating_timer_t *t);
void motor_pwm_init(uint gpio_en);
void motor_set_duty(uint gpio_en, uint16_t level);

// ----------------- ISR DE L'ENCODEUR -----------------
// Appelé à chaque front montant sur ENC_SA

void encoder_isr(uint gpio, uint32_t events) {
    if (gpio == ENC_SA && (events & GPIO_IRQ_EDGE_RISE)) {
        bool sb = gpio_get(ENC_SB);  // lit le canal B

        if (!sb) {
            motor_position++;        // un sens
        } else {
            motor_position--;        // sens opposé
        }
    }
}

// ----------------- TIMER POUR LA VITESSE -----------------
// S'exécute toutes les SPEED_PERIOD_MS ms

bool speed_timer_cb(repeating_timer_t *t) {
    (void)t; // évite un avertissement

    int32_t pos   = motor_position;
    int32_t delta = pos - last_position;
    last_position = pos;

    float period_s = SPEED_PERIOD_MS / 1000.0f;

    // comptages/seconde → RPM
    motor_speed_rpm = (delta / period_s) / ENC_COUNTS_PER_REV * 60.0f;

    printf("Position = %ld  |  Vitesse = %.2f RPM\n",
           (long)pos, motor_speed_rpm);

    return true; // maintient le timer actif
}

// ----------------- FONCTIONS AUXILIAIRES -----------------

// Configure PWM sur la broche EN, SANS allumer le moteur pour l'instant
void motor_pwm_init(uint gpio_en) {
    gpio_set_function(gpio_en, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio_en);

    // Désactive le slice avant de configurer
    pwm_set_enabled(slice_num, false);

    // Configure la fréquence (~20 kHz)
    pwm_set_wrap(slice_num, PWM_WRAP);

    // Commence avec un niveau 0 (moteur arrêté)
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

// Balayage PWM : augmente le niveau par `step` (ex. 50), attend `pause_ms` ms,
// lit la vitesse calculée par l'encodeur et stocke les valeurs jusqu'à atteindre
// un niveau >= target_level. Affiche ensuite la liste des vitesses (RPM).
static void pwm_sweep_record(uint32_t step, uint32_t target_level, uint32_t pause_ms) {
    uint32_t max_steps = (target_level / (step ? step : 1)) + 5;
    if (max_steps < 16) max_steps = 16;
    if (max_steps > 1000) max_steps = 1000;

    float *speeds = malloc(sizeof(float) * max_steps);
    int32_t *positions = malloc(sizeof(int32_t) * max_steps);
    uint32_t *levels = malloc(sizeof(uint32_t) * max_steps);
    if (!speeds || !positions || !levels) {
        printf("Erreur allocation memoire pour le balayage\n");
        free(speeds);
        free(positions);
        free(levels);
        return;
    }

    uint32_t count = 0;
    uint32_t level = 0;
    while (true) {
        if (count >= max_steps) break;
        motor_set_duty(PIN_EN, (uint16_t)level);

        // attend la période demandée pour que le timer calcule la vitesse
        sleep_ms(pause_ms);

        // récupère la vitesse calculée par le timer (RPM) et la position
        float measured = motor_speed_rpm;
        int32_t pos_snapshot = motor_position;

        speeds[count] = measured;
        positions[count] = pos_snapshot;
        levels[count] = level;
        printf("[Sweep] idx=%u Level=%u Pos=%ld Vitesse=%.2f RPM\n", count, level, (long)pos_snapshot, measured);
        count++;

        if (level >= target_level) break;
        level += step;
        if (level > PWM_WRAP) level = PWM_WRAP;
    }

    // Affiche les résultats sous forme de liste structurée
    printf("\nRESULTATS DU BALAYAGE (index, level, position, speed_rpm):\n");
    for (uint32_t i = 0; i < count; ++i) {
        printf("%u, %u, %ld, %.2f\n", i, levels[i], (long)positions[i], speeds[i]);
    }

    free(speeds);
    free(positions);
    free(levels);
}

int main() {
    stdio_init_all();
    // Petit délai pour laisser le temps d'ouvrir le Serial Monitor
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

    // ---------- TIMER POUR LE CALCUL DE VITESSE ----------
    repeating_timer_t speed_timer;
    add_repeating_timer_ms(
        -SPEED_PERIOD_MS,   // négatif => répétitif
        speed_timer_cb,
        NULL,
        &speed_timer
    );

    // ---------- BALAYAGE PWM ET ENREGISTREMENT ----------

    // Définit le sens (0 ou 1 dépend du câblage sur le driver)
    gpio_put(PIN_DIR, 1);

    // Effectue un balayage PWM : pas 50, pause 1000 ms, arrêt >= PWM_WRAP/2 (ici 6249)
    pwm_sweep_record(50u, 6249u, 1000u);

    // Éteint le moteur
    motor_set_duty(PIN_EN, 0);
    printf("Balayage terminé. Moteur éteint.\n");

    // Boucle infinie pour garder le programme actif
    while (true) {
        sleep_ms(500);
    }
}