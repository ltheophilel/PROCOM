// /**
//  * Contrôle de moteur DC avec Pmod HB5 sur Raspberry Pi Pico
//  * Contrôle de la vitesse (PWM) et de la direction
//  */

// // #include <stdio.h>
// // #include <stdbool.h>
// // #include <stdint.h>
// // #include "pico/stdlib.h"
// // #include "motor_control.h"
// // #include "pico/stdlib.h"




// /**
//  * Fonction principale
//  */
// int main() {
//     // Initialise la communication série
//     stdio_init_all();
    
//     // Attendre un peu pour que la série soit prête
//     sleep_ms(2000);
    
//     printf("\n=== Contrôle Moteur DC avec Pmod HB5 ===\n");
//     printf("Initialisation...\n");
    
//     // Initialise les broches
//     init_pins();
    
//     printf("Configuration terminée\n");
//     printf("Broche DIR: GPIO %d\n", DIR_PIN);
//     printf("Broche PWM: GPIO %d\n", PWM_PIN);
//     printf("\n");
    
//     // Boucle principale de démonstration
//     while (true) {
//         // Exemple de séquence de contrôle
        
//         // Avant à 50%
//         printf("\n--- Séquence 1: Avant 50%% ---\n");
//         set_speed_percent(50);
//         set_direction(true);
//         sleep_ms(3000);
        
//         // Arrêt
//         printf("\n--- Arrêt ---\n");
//         stop_motor();
//         sleep_ms(1000);
        
//         // Arrière à 30%
//         printf("\n--- Séquence 2: Arrière 30%% ---\n");
//         set_speed_percent(30);
//         set_direction(true);
//         sleep_ms(3000);
        
//         // Arrêt
//         printf("\n--- Arrêt ---\n");
//         stop_motor();
//         sleep_ms(1000);
        
//         // Avant à 75%
//         printf("\n--- Séquence 3: Avant 75%% ---\n");
//         set_speed_percent(75);
//         set_direction(true);
//         sleep_ms(3000);
        
//         // Arrêt
//         printf("\n--- Arrêt ---\n");
//         stop_motor();
//         sleep_ms(1000);
        
//         // Accélération progressive
//         printf("\n--- Séquence 4: Accélération progressive ---\n");
//         set_direction(true);
//         for (int i = 0; i <= 100; i += 10) {
//             set_speed_percent(i);
//             sleep_ms(200);
//         }
//         sleep_ms(1000);
        
//         // Décélération progressive
//         printf("\n--- Décélération progressive ---\n");
//         for (int i = 100; i > 0; i -= 10) {
//             set_speed_percent((int)i);
//             sleep_ms(200);
//         }
//         stop_motor();
//         sleep_ms(2000);
//     }
    
//     return 0;
// }
