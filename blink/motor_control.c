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

 
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "motor_control.h"

 int slice_num;

 void init_pins(void) {
    // Direction pin
    gpio_init(DIR_PIN);
    gpio_set_dir(DIR_PIN, GPIO_OUT);
    gpio_put(DIR_PIN, 0);

    // PWM pin -> configure function
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(PWM_PIN);

    pwm_config config = pwm_get_default_config();
    // We'll set clkdiv to get desired frequency; assumed system clock default 125 MHz
    pwm_config_set_wrap(&config, PWM_WRAP);
    // choose clock divider 10 -> 125MHz/10/(PWM_WRAP+1) ~= 10000000/(12501) ~ 800 Hz (but this depends)
    // The build below uses the same constants as prior examples; you can tune clkdiv if needed.
    pwm_config_set_clkdiv(&config, 10.0f);
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
    pwm_init(slice_num, &config, true);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PWM_PIN), 0);
}

// Set direction: true = forward, false = reverse
 void set_direction(bool forward) {
    gpio_put(DIR_PIN, forward ? 1 : 0);
}

// speed: 0..100
 void set_speed_percent(int speed_percent) {
    if (speed_percent > 100) speed_percent = 100;
    int level = (int)speed_percent * (int)PWM_WRAP / 100u;
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PWM_PIN), level);
}

 void stop_motor(void) {
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(PWM_PIN), 0);
}

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
