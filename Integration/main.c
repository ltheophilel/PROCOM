#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lib.h"
#include "hardware/i2c.h"
#include "pico/multicore.h"
#include "pico/sync.h"


// Variables globales pour les paramètres du robot (modifiable via IHM)
short pourcentage_Vmax = 30; // en pourcentage de Vmax (environ vitesse en cm/s)
short pourcentage_V_marche_arriere = 60; // en pourcentage de V, vitesse utilisée lors de la recherche de ligne
bool pause = true; // État de pause du robot
float T = 0.3f; // 0.50f; // temps caractéristique pour faire un virage (s). Virage éffectué en 3T environ. Si mode_P, T est calculé dynamiquement en fonction de P et de la vitesse pour que le gain soit constant (T = 1.0f / (P * (pourcentage_Vmax * Vmax / 100.0f))).
float P = 15.0f; // gain proportionnel pour la correction d'angle (T = 1.0f / (P * (pourcentage_Vmax * Vmax / 100.0f)); // en secondes)
bool mode_P = false; // true : mode proportionnel, false : mode fixe
char general_msg[LEN_GENERAL_MSG]; // Message général pour la communication
short SEUIL = 128; // seuil de binarisation pour le traitement d'image
int PROFONDEUR = 30; // Profondeur pour le calcul de l'angle

// Variables passées entre les deux coeurs pour transmission
signed int v_mot_droit; // Vitesse du moteur droit en RPM
signed int v_mot_gauche; // Vitesse du moteur gauche en RPM
double angle; // Angle détecté de la ligne
double p; // Ordonnée à l'origine de la droite détectée
double m; // Pente de la droite détectée
double p_aplati; // Paramètres après aplatissement homographique
double m_aplati;
double angle_aplati;
double angle_utilise; // Angle effectivement utilisé pour le contrôle

// Fonction pour interpréter les commandes reçues via TCP et ajuster les paramètres du robot
void interpretCommand(TCP_SERVER_T *state, const char* command) {
    command = trim_whitespace_divers((char *)command);
    printf("Interpreted Command: '%s'\n", command);
    if (strcmp(command, "LED ON") == 0)
    {
        pico_set_led(true);
        snprintf(general_msg, LEN_GENERAL_MSG, "LED ON");
    }
    else if (strcmp(command, "LED OFF") == 0)
    {
        pico_set_led(false);
        snprintf(general_msg, LEN_GENERAL_MSG, "LED OFF");
    }
    else if (strcmp(command, "STOP") == 0)
    {
        pause = true;
        snprintf(general_msg, LEN_GENERAL_MSG, "Robot PAUSED");
    }
    else if (strcmp(command, "GO") == 0)
    {
        pause = false;
        snprintf(general_msg, LEN_GENERAL_MSG, "Robot RUNNING");
    }
    else if (command[0] == 'V' && estNombreEntier(command + 1))
    {
        int v_percentage = atoi(command + 1);
        if (v_percentage >= 0 && v_percentage <= 100) {
            pourcentage_Vmax = (short)v_percentage;
            snprintf(general_msg, LEN_GENERAL_MSG, "Vmax set to %d%%", pourcentage_Vmax);
        } else {
            snprintf(general_msg, LEN_GENERAL_MSG, "Error: V%% out of range");
        }
    }
    else if (command[0] == 'T' && estNombreEntier(command + 1))
    {
        T = atoi(command + 1) / 1000.0f; // Conversion en secondes
        mode_P = false; // Désactiver le mode proportionnel
        snprintf(general_msg, LEN_GENERAL_MSG, "T set to %.3f s", T);
    }
    else if (command[0] == 'P' && estNombreEntier(command + 1))
    {
        P = atoi(command + 1) / 1000.0f; 
        mode_P = true; // Activer le mode proportionnel
        snprintf(general_msg, LEN_GENERAL_MSG, "P set to %.3f", P);
    }
    else if (command[0] == 'S' && estNombreEntier(command + 1))
    {
        SEUIL = (short)atoi(command + 1); // Seuil de binarisation
        snprintf(general_msg, LEN_GENERAL_MSG, "SEUIL set to %d", SEUIL);
    }
    else if (command[0] == 'D' && estNombreEntier(command + 1))
    {
        PROFONDEUR = (short)atoi(command + 1); // Profondeur pour calcul d'angle
        snprintf(general_msg, LEN_GENERAL_MSG, "PROFONDEUR set to %d", PROFONDEUR);
    }
    else if (command[0] == 'R' && estNombreEntier(command + 1))
    {
        pourcentage_V_marche_arriere = (short)atoi(command + 1); // Pourcentage de la vitesse maximale pour la marche arrière
        snprintf(general_msg, LEN_GENERAL_MSG, "R set to %d%%", pourcentage_V_marche_arriere);
    }
    else
    {
        // Réponse simple : renvoyer ce qu'on a reçu
        snprintf(general_msg, LEN_GENERAL_MSG, "Unknown : %s", command);
    }
}

static int cpt_envoi = 0; // compte le nombre d'envois effectués pour le debug

// Entrée du cœur 1 : gestion du Wi-Fi, TCP et OLED
void core1_entry() {
    // Initialisation Wi-Fi
    err_t connect_success = wifi_auto_connect();
    if (connect_success != ERR_OK){
        printf("Échec de la connexion Wi-Fi.\n");
        pause = false;
    } else pico_set_led(true);
    printf(connect_success == ERR_OK ? "Connecté au Wi-Fi.\n" : "Continuing without Wi-Fi...\n");
    TCP_SERVER_T* state;
    uint8_t rx_buffer[BUF_SIZE];
    if (connect_success == ERR_OK) state = tcp_server_start();

    // Initialisation OLED
    initOLED();
    renderSymbol(RPI,1,56); // Afficher le symbole RPi
    renderIPString(IP4ADDR);
    if (connect_success == ERR_OK) {
        uint8_t *coded_image; // Image codée pour transmission
        coded_image = (uint8_t*)malloc(1024*sizeof(uint8_t)); // Allocation pour l'image codée (taille max possible)
        static uint64_t t_us_core_1_beginning_loop;
        while (true) {
            printf("IP Address: %s\n", IP4ADDR);
            cpt_envoi++;
            t_us_core_1_beginning_loop = time_us_64();
            
            int received = tcp_server_receive(state, rx_buffer, BUF_SIZE);

            if (received > 0) {
                // Si message reçu, l'interpréter
                rx_buffer[received] = '\0'; // Ajouter un '\0' pour créer une chaîne de caractères
                printf("Reçu du client : %s\n", rx_buffer);
                interpretCommand(state, (const char *)rx_buffer);
            }
            
            err_t err;
        
            // Coder l'image et envoyer tout en un paquet
            sleep_ms(20);
            byte_to_bit_for_transmission(*get_outbuf_from_core(1),
                                        coded_image,
                                        MAX_WIDTH, MAX_HEIGHT, SEUIL);
            
            printf("Sending all-in-one packet...\n");
            err = tcp_server_send_all_in_one(state,
                                        general_msg,
                                        v_mot_droit,
                                        v_mot_gauche,
                                        p,
                                        m,
                                        angle,
                                        p_aplati,
                                        m_aplati,
                                        angle_aplati,
                                        coded_image,
                                        600); 
            // Reset general_msg
            snprintf(general_msg, LEN_GENERAL_MSG, ""); 
            general_msg[0] = '\0';
            
            printf("TCP send time (us): %llu\n", time_us_64() - t_us_core_1_beginning_loop);
            if (err != ERR_OK) {
                printf("Erreur d'envoi de l'image : %d\n", err);
            }
            // Échange des buffers pour synchronisation avec le cœur 0
            uint8_t** pointer_to_outbuf = get_outbuf_from_core(1);
            core_ready_to_swap(1, true);
            while (pointer_to_outbuf == get_outbuf_from_core(1)) {
                sleep_ms(10);
            }
            printf("Core 1: Processing time (us): %llu\n", time_us_64() - t_us_core_1_beginning_loop);
        
        tight_loop_contents();
        }
    }
}

// Entrée du cœur 0 : gestion de la caméra, traitement d'image et contrôle des moteurs
void core0_entry()
{
    // Initialisation moteurs
    printf("Initialisation des moteurs\n");
    init_all_motors_and_encoders();

    /* INITIALISATION CAMERA */
    static uint8_t *frame_buffer, *bw_outbuf; // Buffers pour l'image brute et binarisée
    struct camera camera;
    init_camera(); // initialisation des GPIO pour la caméra
    struct camera_platform_config platform = create_camera_platform_config();

    /* Choix Format */
    uint16_t width, height;
    OV7670_size size;
    int division = 0; // 0 : DIV 8, 1 : DIV 4, ...
    int format_out = choix_format(division, &width, &height, &size);

    printf("Camera to be initialised\n");
    if (camera_init(&camera, &platform, size)) {
        printf("Erreur d'initialisation de la caméra\n"); 
        pico_blink_led(5, 500);
        // return 1;
    } else {
        printf("Camera initialised\n");
    }

    /* Creation Buffers Camera */
    // On prépare les 2 buffers car ils sont tout les deux utilisés successivement
    creation_buffers_camera(&frame_buffer, get_outbuf_from_core(0),
                           &bw_outbuf, width, height);

    creation_buffers_camera(&frame_buffer, get_outbuf_from_core(1),
                           &bw_outbuf, width, height);

    err_t err;
    static uint64_t t_us_core_0_beginning_loop;
    while (true) {
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll(); // Gestion des événements Wi-Fi si nécessaire
        #endif
        
        t_us_core_0_beginning_loop = time_us_64();
        core_ready_to_swap(0, false); // Indiquer que le cœur 0 n'est pas prêt pour l'échange
        camera_capture_blocking(&camera, frame_buffer, width, height); // Capture d'image bloquante
        
        // Traitement d'image : binarisation
        int seuillage_out = seuillage(frame_buffer, bw_outbuf,
                                      width, height, SEUIL);
        *get_outbuf_from_core(0) = bw_outbuf; // Utiliser l'image binarisée pour envoi optimisé
        core_ready_to_swap(0, true); // Prêt pour l'échange de buffers

        if (ligne_detectee(bw_outbuf, width, height) == 0 && !pause)
        {
            // Ligne non détectée : rechercher la ligne en marche arrière
            int* vitesses = get_vitesse_mot(Vmax * pourcentage_Vmax / 100.0, angle_utilise, T); // Calcul des vitesses en RPM
            chercher_ligne(vitesses[0], vitesses[1], angle_utilise, pourcentage_V_marche_arriere);
        }
        else
        {            
            // Ligne détectée : calculer l'angle et ajuster les vitesses
            double* apm = trouver_angle(bw_outbuf, width, height, PROFONDEUR); // Retourne angle, p, m
            angle = apm[0]; p = apm[1]; m = apm[2];
            free(apm);
            double* apm_aplati = aplatir(angle, p, m, PROFONDEUR); // Appliquer transformation homographique
            angle_aplati = apm_aplati[0]; p_aplati = apm_aplati[1]; m_aplati = apm_aplati[2];
            free(apm_aplati);
            angle = PI*angle/180; // Conversion degrés vers radians
            angle_aplati = PI*angle_aplati/180;
            angle_utilise = angle_aplati; // Utiliser l'angle après aplatissement
            
            if (mode_P) {
                T = 1.0f / (P * (pourcentage_Vmax * Vmax / 100.0f)); // Calcul dynamique de T en mode proportionnel en secondes
            }
            int* vitesses = get_vitesse_mot(Vmax * pourcentage_Vmax / 100.0, angle_utilise, T); // Calcul des vitesses des moteurs en rpm
            v_mot_droit = vitesses[0];
            v_mot_gauche = vitesses[1];
            printf("Angle: %.2f rad, V droite: %d rpm, V gauche: %d rpm\n",
                angle_utilise, v_mot_droit, v_mot_gauche);
            
            if (!pause) {
                // Appliquer les vitesses aux moteurs
                motor_define_direction_from_pwm(v_mot_droit, v_mot_gauche);
                motor_set_rpm(&moteur0, abs_double(v_mot_droit));
                motor_set_rpm(&moteur1, abs_double(v_mot_gauche)); 
            }
            else {
                // Robot en pause : arrêter les moteurs
                motor_set_rpm(&moteur0, 0.0);
                motor_set_rpm(&moteur1, 0.0);
            }
        }

        
        printf("Core 0: Processing time (us): %llu\n", time_us_64() - t_us_core_0_beginning_loop);

        // FPS max → pas de pause
        tight_loop_contents();
    }

    // Nettoyage en fin de programme
    // tcp_server_stop(state);
    pico_set_led(false);
    cyw43_arch_deinit();
    free(frame_buffer);
    free(*get_outbuf_from_core(0));
    free(*get_outbuf_from_core(1));
    free(bw_outbuf);
}

// BUFF_SIZE défini dans tcp_server.h

int main() {
    // Fonction principale : initialisation et lancement des cœurs
    stdio_init_all();
    sleep_ms(500);
    multicore_launch_core1(core1_entry); // Lancer le cœur 1
    core0_entry(); // Exécuter le cœur 0 (boucle infinie)
    return 0;
}
