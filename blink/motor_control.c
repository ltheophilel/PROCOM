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
#include <stdint.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "tusb.h" // tinyusb status checks (tud_cdc_connected)

// ----------------- DÉFINITION DES BROCCHES -----------------

// Connexion avec le driver (PmodHB5, par exemple)
#define PIN_DIR 14      // DIR du driver
#define PIN_EN  15      // EN / ENA du driver

// Encodeur du moteur
#define ENC_SA  16      // canal A (génère une interruption)
#define ENC_SB  17      // canal B (sens)

// LED interne (Pico on-board LED, GPIO 25 sur la plupart des cartes)
#define LED_PIN 25

// ----------------- PARAMÈTRES PWM -----------------

// ~20 kHz : 125 MHz / (6249 + 1) ≈ 20 kHz
#define PWM_WRAP        6249
// Duty cycle élevé pour garantir que le moteur tourne (≈ 100%)
#define PWM_DUTY_START  6248    

// ----------------- PARAMÈTRES ENCODEUR / VITESSE -----------------

#define SPEED_PERIOD_MS     1000        // calcul de vitesse toutes les 1 s
#define ENC_COUNTS_PER_REV  318.0f      // Valeur calibrée (empirique)

// ----------------- VARIABLES GLOBALES -----------------

volatile int32_t motor_position   = 0;   // position en comptages
volatile int32_t last_position    = 0;   // position à la dernière mesure
volatile float   motor_speed_rpm  = 0.0f;
volatile bool    menu_active      = false; // true quand on attend une saisie au menu (évite d'afficher les logs)
volatile bool    speed_updated    = false; // true quand un nouveau calcul de vitesse est disponible
volatile bool    operation_active = false; // true pendant un balayage/une recherche (évite d'afficher les logs)

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

    // comptages/seconde → RPM (toujours calculé)
    motor_speed_rpm = ((float)delta / period_s) / ENC_COUNTS_PER_REV * 60.0f;

    // marque la nouvelle vitesse disponible — évite printf depuis l'interruption
    speed_updated = true;

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

// Read an integer from serial input (non-blocking with timeout; returns -1 on timeout/cancel)
static int read_input_int(void) {
    char buffer[20];
    int idx = 0;
    // Indique au timer d'éviter d'afficher pendant la saisie
    menu_active = true;
    printf("[DBG] Entering read_input_int()\n");
    printf("> ");
    fflush(stdout);

    int idle_ticks = 0;
    const int tick_us = 100000; // 100 ms per tick
    const int max_idle_ticks = 200; // 200 ticks -> 20 seconds timeout
    const int dot_interval = 5; // print dot every ~500 ms
    int conn_check_ticks = 0; // used to check USB connection periodically

    while (idx < (int)sizeof(buffer) - 1) {
        int ch = getchar_timeout_us(tick_us);
        if (ch == PICO_ERROR_TIMEOUT) {
            idle_ticks++;
            conn_check_ticks++;
            if (idle_ticks % dot_interval == 0) {
                printf(".");
                fflush(stdout);
            }
            // Every ~2 seconds, verify USB CDC connection status
            if (conn_check_ticks >= 20) {
                conn_check_ticks = 0;
                int usb_ok = tud_cdc_connected() ? 1 : 0;
                static bool usb_warned = false;
                if (!usb_ok) {
                    if (!usb_warned) {
                        printf("\n[WARN] USB CDC not connected (tud_cdc_connected=0). Check Serial Monitor, cable or drivers.\n");
                        fflush(stdout);
                        usb_warned = true;
                    }
                    // don't cancel here; continue waiting so the user can try again
                } else {
                    if (usb_warned) {
                        printf("\n[INFO] USB CDC connected\n");
                        fflush(stdout);
                        usb_warned = false;
                    }
                }
            }
            if (idle_ticks >= max_idle_ticks) {
                printf("\n[Annulé : timeout]\n");
                menu_active = false;
                return -1;
            }
            continue;
        }
        if (ch == '\n' || ch == '\r') {
            buffer[idx] = '\0';
            break;
        }
        if (ch >= '0' && ch <= '9') {
            buffer[idx++] = (char)ch;
            printf("%c", ch);  // Echo back
            fflush(stdout);
        } else if ((ch == '-' || ch == '+') && idx == 0) {
            buffer[idx++] = (char)ch;
            printf("%c", ch);
            fflush(stdout);
        } else if (ch == 3) { // Ctrl-C
            printf("\n[Annulé]\n");
            menu_active = false;
            printf("[DBG] Exiting read_input_int() with -1 (Ctrl-C)\n");
            return -1;
        } else {
            // Affiche un message debug si autres caractères arrivent (utile pour diagnostiquer)
            printf("[DBG] recv ch=%d\n", ch);
            fflush(stdout);
        }
    }

    printf("\n");
    menu_active = false; // saisie terminée, réactive l'affichage

    if (idx == 0) {
        printf("[DBG] Exiting read_input_int() empty input -> 0\n");
        return 0; // empty input -> 0
    }
    printf("[DBG] Exiting read_input_int() -> %s\n", buffer);
    return atoi(buffer);
}

// Interactive menu
static void show_menu(void) {
    printf("\n");
    printf("========================================\n");
    printf("       CONTROLE MOTEUR DC - MENU\n");
    printf("========================================\n");
    printf("1. Balayage complet PWM (0 -> 6249)\n");
    printf("2. Trouver PWM pour une vitesse cible\n");
    printf("3. Quitter (arreter moteur)\n");
    printf("========================================\n");
    printf("Choisissez [1-3] : (Tapez le numéro puis ENTER — si rien, attendez 20s pour annuler) ");
}

// Wait for user to press ENTER (timeout_ms ms). Returns true if ENTER pressed, false on timeout.
static bool wait_for_enter(uint32_t timeout_ms) {
    printf("Appuyez sur ENTREE pour revenir au menu (timeout %lu s)...\n", (unsigned long)(timeout_ms / 1000));
    fflush(stdout);

    uint32_t ticks = timeout_ms / 100; // 100 ms ticks
    for (uint32_t t = 0; t < ticks; ++t) {
        int c = getchar_timeout_us(100000);
        if (c == PICO_ERROR_TIMEOUT) continue;
        if (c == '\n' || c == '\r') return true;
        // ignore other chars
    }
    printf("Retour automatique au menu.\n");
    return false;
}

// ----------------- MAIN -----------------

// Balayage PWM : augmente le niveau par `step` (ex. 25), attend `pause_ms` ms,
// lit la vitesse calculée par l'encodeur et stocke les valeurs jusqu'à atteindre
// un niveau >= target_level. Affiche ensuite la liste des vitesses (RPM).
static void pwm_sweep_record(uint32_t step, uint32_t target_level, uint32_t pause_ms) {
    operation_active = true;

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
        operation_active = false;
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
        float measured_signed = motor_speed_rpm;
        float measured = fabsf(measured_signed); // use absolute speed for analysis
        int32_t pos_snapshot = motor_position;

        speeds[count] = measured;
        positions[count] = pos_snapshot;
        levels[count] = level;
        printf("[Sweep] idx=%u Level=%u Pos=%ld Vitesse=%.2f RPM (signed %.2f)\n", count, level, (long)pos_snapshot, measured, measured_signed);
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

    // Pause pour laisser le temps de lire les résultats
    wait_for_enter(30000);

    free(speeds);
    free(positions);
    free(levels);

    operation_active = false;
}

// Find PWM level for a desired target RPM using a linear interpolation
// between measured points from a sweep with step 'step'.
// Returns an estimated PWM level (0..PWM_WRAP).
static uint32_t pwm_find_for_rpm(float target_rpm, uint32_t step, uint32_t target_level, uint32_t pause_ms) {
    operation_active = true;

    uint32_t max_steps = (target_level / (step ? step : 1)) + 5;
    if (max_steps < 16) max_steps = 16;
    if (max_steps > 2000) max_steps = 2000;

    float *speeds = malloc(sizeof(float) * max_steps);
    uint32_t *levels = malloc(sizeof(uint32_t) * max_steps);
    if (!speeds || !levels) {
        printf("Erreur allocation memoire pour pwm_find_for_rpm\n");
        free(speeds);
        free(levels);
        operation_active = false;
        return 0;
    }

    // Do a sweep and record speeds
    uint32_t count = 0;
    uint32_t level = 0;
    while (true) {
        if (count >= max_steps) break;
        motor_set_duty(PIN_EN, (uint16_t)level);
        sleep_ms(pause_ms);

        float measured_signed = motor_speed_rpm;
        float measured = fabsf(measured_signed);
        speeds[count] = measured;
        levels[count] = level;
        printf("[Finder] idx=%u Level=%u Vitesse=%.2f RPM (signed %.2f)\n", count, level, measured, measured_signed);
        count++;

        if (level >= target_level) break;
        level += step;
        if (level > PWM_WRAP) level = PWM_WRAP;
    }

    // If no valid points, bail
    if (count == 0) {
        free(speeds);
        free(levels);
        operation_active = false;
        return 0;
    }

    // If target is below min or above max, clamp to nearest
    float min_speed = speeds[0];
    float max_speed = speeds[0];
    for (uint32_t i = 1; i < count; ++i) {
        if (speeds[i] < min_speed) min_speed = speeds[i];
        if (speeds[i] > max_speed) max_speed = speeds[i];
    }

    uint32_t estimated_level = levels[count-1];

    if (target_rpm <= min_speed) {
        estimated_level = levels[0];
        printf("Target RPM below measured range (%.2f <= %.2f). Using level %u\n", target_rpm, min_speed, estimated_level);
    } else if (target_rpm >= max_speed) {
        estimated_level = levels[count-1];
        printf("Target RPM above measured range (%.2f >= %.2f). Using level %u\n", target_rpm, max_speed, estimated_level);
    } else {
        // find bracketing interval (assume monotonic increasing or decreasing)
        int idx = -1;
        for (uint32_t i = 0; i + 1 < count; ++i) {
            float a = speeds[i];
            float b = speeds[i+1];
            if ((a <= target_rpm && target_rpm <= b) || (b <= target_rpm && target_rpm <= a)) {
                idx = (int)i;
                break;
            }
        }

        if (idx < 0) {
            // fallback to nearest
            float best_diff = fabsf(speeds[0] - target_rpm);
            int best = 0;
            for (uint32_t i = 1; i < count; ++i) {
                float d = fabsf(speeds[i] - target_rpm);
                if (d < best_diff) { best_diff = d; best = i; }
            }
            estimated_level = levels[best];
            printf("No bracket found; using nearest level %u (speed=%.2f)\n", estimated_level, speeds[best]);
        } else {
            // linear interpolate between idx and idx+1
            float s0 = speeds[idx];
            float s1 = speeds[idx+1];
            uint32_t l0 = levels[idx];
            uint32_t l1 = levels[idx+1];
            float ratio = 0.0f;
            if (s1 != s0) ratio = (target_rpm - s0) / (s1 - s0);
            float est = (float)l0 + ratio * (float)(l1 - l0);
            if (est < 0.0f) est = 0.0f;
            if (est > (float)PWM_WRAP) est = (float)PWM_WRAP;
            estimated_level = (uint32_t)est;
            printf("Interpolated level=%.1f from [%u:%.2f] - [%u:%.2f]\n", est, l0, s0, l1, s1);
        }
    }

    // Verify by setting estimated level and measuring
    motor_set_duty(PIN_EN, (uint16_t)estimated_level);
    sleep_ms(pause_ms);
    float verify_signed = motor_speed_rpm;
    float verify_abs = fabsf(verify_signed);
    printf("[Verify] Set level=%u -> Measured RPM=%.2f (signed %.2f)\n", estimated_level, verify_abs, verify_signed);

    free(speeds);
    free(levels);
    return estimated_level;
}

int main() {
    stdio_init_all();
    // Petit délai (plus court) et message d'attente pour que l'utilisateur ouvre le Serial Monitor
    sleep_ms(500);

    printf("\n=== MOTEUR DC - PMOD HB5 (Pico 2) ===\n");
    printf("ENC_COUNTS_PER_REV = 159.0 (calibré)\n");
    printf("PWM_WRAP = 6249 (~20 kHz)\n\n");

    // Indiquer que l'on attend l'ouverture du moniteur série (jusqu'à ~5s)
    printf("Attente ouverture du moniteur série (5s). Ouvrez le Serial Monitor maintenant...\n");
    fflush(stdout);
    int wait_ticks = 0;
    const int max_wait_ticks = 50; // 50 * 100 ms = 5 s
    while (wait_ticks < max_wait_ticks) {
        int c = getchar_timeout_us(100000); // 100 ms
        if (c != PICO_ERROR_TIMEOUT) break; // on a lu quelque chose -> host connecté
        if (wait_ticks % 5 == 0) {
            printf(".");
            fflush(stdout);
        }
        wait_ticks++;
    }
    printf("\nDémarrage du menu...\n\n");

    // --- Test de réception (diagnostic) : 5s pour taper des caractères et vérifier qu'ils arrivent ---
    printf("Test reception: tapez quelques caracteres (5 s). Vous verrez chaque octet reçu affiché.\n");
    fflush(stdout);
    const int test_ticks = 50; // 50 * 100 ms = 5 s
    for (int t = 0; t < test_ticks; ++t) {
        int c = getchar_timeout_us(100000);
        if (c == PICO_ERROR_TIMEOUT) continue;
        if (c == '\r' || c == '\n') {
            printf("[RX] newline (\n)\n");
        } else if (c >= 32 && c < 127) {
            printf("[RX] ch=%d '%c'\n", c, (char)c);
        } else {
            printf("[RX] ch=%d\n", c);
        }
        fflush(stdout);
    }
    printf("Fin test reception\n\n");
    fflush(stdout);

    // Affiche le statut TinyUSB CDC (utile pour diagnostiquer les problèmes d'entrée)
    printf("[USB] tud_cdc_connected() = %d\n", tud_cdc_connected() ? 1 : 0);
    fflush(stdout);


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
        -SPEED_PERIOD_MS,
        speed_timer_cb,
        NULL,
        &speed_timer
    );

    // Définit le sens (0 ou 1 dépend du câblage sur le driver)
    gpio_put(PIN_DIR, 1);

    // Initialise LED onboard pour indiquer que le firmware est vivant
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // ---------- MENU PRINCIPAL ----------
    int choice = 0;
    while (true) {
        show_menu();
        fflush(stdout);
        choice = read_input_int();

        if (choice == -1) {
            printf("Saisie annulée (timeout). Retour au menu.\n\n");
            continue;
        }

        switch (choice) {
            case 1:
                // Balayage complet
                printf("\n[Option 1] Balayage complet PWM (0 -> 6249, step 25)\n");
                printf("Cela prendra ~4 minutes. Démarrage...\n\n");
                pwm_sweep_record(25u, 6249u, 1000u);
                printf("\nBalayage terminé.\n");
                break;

            case 2:
                // Trouver RPM pour cible
                printf("\n[Option 2] Trouver PWM pour une vitesse cible\n");
                printf("Entrez la RPM cible (ex: 150) : ");
                fflush(stdout);
                int target_rpm = read_input_int();
                if (target_rpm == -1) {
                    printf("Saisie annulée. Retour au menu.\n");
                } else if (target_rpm < 0 || target_rpm > 300) {
                    printf("RPM hors plage (0-300). Annulé.\n");
                } else {
                    printf("\nRecherche PWM pour RPM=%.0f...\n", (float)target_rpm);
                    printf("Cela prendra ~4 minutes. Veuillez patienter...\n\n");
                    uint32_t pwm_level = pwm_find_for_rpm((float)target_rpm, 25u, 6249u, 1000u);
                    printf("\n[Résultat] RPM=%.0f -> PWM level=%u\n\n", (float)target_rpm, pwm_level);
                }
                break;

            case 3:
                // Quitter
                printf("\n[Option 3] Arrêt du moteur et sortie.\n");
                motor_set_duty(PIN_EN, 0);
                printf("Moteur arrêté. À bientôt!\n");
                while (true) {
                    sleep_ms(1000);
                }
                break;

            default:
                printf("Choix invalide. Réessayez.\n");
                break;
        }

            // affiche la dernière mesure si disponible (sécurité : hors interruption)
        if (speed_updated && !menu_active && !operation_active) {
            printf("Position = %ld  |  Vitesse = %.2f RPM\n",
                   (long)motor_position, motor_speed_rpm);
            speed_updated = false;
        }

        // Indicateur visuel : clignote LED onboard à chaque boucle principale
        static bool led_state = false;
        led_state = !led_state;
        gpio_put(LED_PIN, led_state ? 1 : 0);

        printf("\n");
        sleep_ms(1000);
    }

    return 0;
}