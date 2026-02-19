#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lib.h"
#include "hardware/i2c.h"
#include "pico/multicore.h"
#include "pico/sync.h"
// #include <jansson.h>

#define Vmax MAX_RPM*R*(2*PI)/60.0f // conversion rpm -> m/s. Environ 1 m/s
#define F 12.5 // Hz


#define OPTIMIZED_SEND true

short pourcentage_Vmax = 20; // en pourcentage de Vmax (environ vitesse en cm/s)
short pourcentage_V_marche_arriere = 60; // en pourcentage de V, vitesse utilisée lors de la recherche de ligne
bool pause = true;
float T = 0.3f; // 0.50f; // temps caractéristique pour faire un virage (s). Virage éffectué en 3T environ. Si mode_P, T est calculé dynamiquement en fonction de P et de la vitesse pour que le gain soit constant (T = 1.0f / (P * (pourcentage_Vmax * Vmax / 100.0f))).
float P = 15.0f; // gain proportionnel pour la correction d'angle (T = 1.0f / (P * (pourcentage_Vmax * Vmax / 100.0f)); // en secondes)
bool mode_P = false; // true : mode proportionnel, false : mode fixe
char general_msg[LEN_GENERAL_MSG];
short SEUIL = 128; // seuil de binarisation pour le traitement d'image
int PROFONDEUR = 30;

static char  str_v_mot[LEN_GENERAL_MSG];
static uint64_t t_us_core_0_beginning_loop;
static uint64_t t_us_core_1_beginning_loop;
signed int v_mot_droit;
signed int v_mot_gauche;
double angle;
double p;
double m;
double p_aplati;
double m_aplati;
double angle_aplati;
double angle_utilise;
uint32_t debut = 0; // Pour gérer le timer de recherche de ligne

static uint8_t* outbuf1;
static uint8_t* outbuf2;
static bool toggle_buf = false;
static uint8_t *frame_buffer, *bw_outbuf;
uint8_t *coded_image;
uint16_t len_coded_image;

static mutex_t multicore_lock;

// échange les buffers d'image entre les deux cœurs   
static uint8_t** get_outbuf_from_core(bool core) {
    uint8_t** outbuf = (toggle_buf == core) ? &outbuf1 : &outbuf2;
    return outbuf;
}
// gestion de la synchronisation entre les deux cœurs
static bool core_0_ready = false;
static bool core_1_ready = false;
static void core_ready_to_swap(bool core, bool ready) {
    switch (core) {
        case 0:
            core_0_ready = ready;
            break;
        case 1:
            core_1_ready = ready;
            break;
    }
    if (core_0_ready && core_1_ready)
    {
        printf("Core 0 using buffer %d, Core 1 using buffer %d\n", toggle_buf ? 0 : 1, toggle_buf ? 1 : 0);
        toggle_buf = !toggle_buf;
        core_0_ready = false;
        core_1_ready = false;
    }
}

void interpretCommand(TCP_SERVER_T *state, const char* command) {
    // Implémentez ici l'interprétation des commandes reçues
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
        T = atoi(command + 1) / 1000.0f; // en secondes
        mode_P = false;
        snprintf(general_msg, LEN_GENERAL_MSG, "T set to %.3f s", T);
    }
    else if (command[0] == 'P' && estNombreEntier(command + 1))
    {
        P = atoi(command + 1) / 1000.0f; 
        mode_P = true;
        snprintf(general_msg, LEN_GENERAL_MSG, "P set to %.3f", P);
    }
    else if (command[0] == 'S' && estNombreEntier(command + 1))
    {
        SEUIL = (short)atoi(command + 1); 
        snprintf(general_msg, LEN_GENERAL_MSG, "SEUIL set to %d", SEUIL);
    }
    else if (command[0] == 'D' && estNombreEntier(command + 1))
    {
        PROFONDEUR = (short)atoi(command + 1); 
        snprintf(general_msg, LEN_GENERAL_MSG, "PROFONDEUR set to %d", PROFONDEUR);
    }
    else if (command[0] == 'R' && estNombreEntier(command + 1))
    {
        pourcentage_V_marche_arriere = (short)atoi(command + 1); 
        snprintf(general_msg, LEN_GENERAL_MSG, "vitesse_marche_arriere set to %d%%", pourcentage_V_marche_arriere);
    }
    else
    {
        // Réponse simple : renvoyer ce qu'on a reçu
        snprintf(general_msg, LEN_GENERAL_MSG, "Unknown : %s", command);
    }
}

static int cpt_envoi = 0;

void core1_entry() {
    // Initialisation Wi-Fi
    err_t connect_success = wifi_auto_connect();
    if (connect_success != ERR_OK)
    {
        printf("Échec de la connexion Wi-Fi.\n");
        pause = false;
    } else pico_set_led(true);
    printf(connect_success == ERR_OK ? "Connecté au Wi-Fi.\n" : "Continuing without Wi-Fi...\n");
    TCP_SERVER_T* state;
    uint8_t rx_buffer[BUF_SIZE];
    if (connect_success == ERR_OK) state = tcp_server_start();

    // Initialisation OLED
    initOLED();
    renderSymbol(RPI,1,56); // Render the Rpi Symbol
    renderIPString(IP4ADDR);
    if (connect_success == ERR_OK) {
        while (true) {
            cpt_envoi++;
            t_us_core_1_beginning_loop = time_us_64();
            
            int received = tcp_server_receive(state, rx_buffer, BUF_SIZE);

            if (received > 0) {
                // Si message reçu
                rx_buffer[received] = '\0'; // Ajouter un '\0' pour créer une chaîne de caractères
                printf("Reçu du client : %s\n", rx_buffer);
                interpretCommand(state, (const char *)rx_buffer);
            }
            
            err_t err;
            if (!OPTIMIZED_SEND) {
                sleep_ms(10);
                snprintf(str_v_mot, sizeof(str_v_mot), "%d", v_mot_droit);
                tcp_server_send(state, str_v_mot, PACKET_TYPE_MOT_0);
                snprintf(str_v_mot, sizeof(str_v_mot), "%d", v_mot_gauche);
                tcp_server_send(state, str_v_mot, PACKET_TYPE_MOT_1);
                snprintf(str_v_mot, sizeof(str_v_mot), "%.6f", 180*angle/PI);
                tcp_server_send(state, str_v_mot, PACKET_TYPE_GENERAL);

                err = tcp_send_large_img(state, *get_outbuf_from_core(1), MAX_WIDTH*MAX_HEIGHT);
            } else {
                printf("Preparing to send all-in-one packet...\n");
                sleep_ms(20);
                // snprintf(str_v_mot, sizeof(str_v_mot), "%.6f", 180*angle/PI);
                // if (general_msg[0] == '\0') {
                //     snprintf(general_msg, LEN_GENERAL_MSG, "%u", cpt_envoi);
                // }
                // seuillage_pour_transmission(*get_outbuf_from_core(1),
                //                             coded_image,
                //                             MAX_WIDTH, MAX_HEIGHT, &len_coded_image, SEUIL); // RLE
                byte_to_bit_for_transmission(*get_outbuf_from_core(1),
                                            coded_image,
                                            MAX_WIDTH, MAX_HEIGHT, SEUIL);
                
                // printf("Coded image length: %d bytes\n", len_coded_image);
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
                                            600); //len_coded_image);
                snprintf(general_msg, LEN_GENERAL_MSG, ""); // Reset general_msg
                general_msg[0] = '\0'; // Reset general_msg
            }
            
            printf("TCP send time (us): %llu\n", time_us_64() - t_us_core_1_beginning_loop);
            if (err != ERR_OK) {
                printf("Erreur d'envoi de l'image : %d\n", err);
            }
            uint8_t** pointer_to_outbuf = get_outbuf_from_core(1);
            core_ready_to_swap(1, true);
            // printf("Core 1: Waiting for swap...\n");
            while (pointer_to_outbuf == get_outbuf_from_core(1)) {
                sleep_ms(10);
            }
            printf("Core 1: Processing time (us): %llu\n", time_us_64() - t_us_core_1_beginning_loop);
        
        tight_loop_contents();
        }
    }
}


void core0_entry()
{
    // Initialisation moteurs
    printf("Initialisation des moteurs\n");
    init_all_motors_and_encoders();

    /* INITIALISATION CAMERA */
    struct camera camera;
    init_camera();
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
    } 
    printf("Camera initialised\n");

    /* Creation Buffers Camera */
    creation_buffers_camera(&frame_buffer, get_outbuf_from_core(0),
                           &bw_outbuf, width, height);

    creation_buffers_camera(&frame_buffer, get_outbuf_from_core(1),
                           &bw_outbuf, width, height);

    err_t err;
    
    while (true) {
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
        #endif
        
        t_us_core_0_beginning_loop = time_us_64();
        core_ready_to_swap(0, false);
        camera_capture_blocking(&camera, frame_buffer, width, height);
        // printf("Capture time (us): %llu\n", time_us_64() - t_us_core_0_beginning_loop);

        // Extraire Y seulement
        // for (int px = 0; px < width * height; px++)
        //     (*get_outbuf_from_core(0))[px] = frame_buffer[px * 2]; 
        if (!OPTIMIZED_SEND) *get_outbuf_from_core(0) = frame_buffer;
        
        // Traitement
        int seuillage_out = seuillage(frame_buffer, bw_outbuf,
                                      width, height, SEUIL);
        if (OPTIMIZED_SEND) *get_outbuf_from_core(0) = bw_outbuf;
        core_ready_to_swap(0, true);

        if (ligne_detectee(bw_outbuf, width, height) == 0 && !pause)
        {
            // Ligne non détectée
            int* vitesses = get_vitesse_mot(Vmax * pourcentage_Vmax / 100.0, angle_utilise, T); // en rpm
            chercher_ligne(vitesses[0], vitesses[1], angle_utilise, pourcentage_V_marche_arriere);
        }
        else
        {            
            double* apm = trouver_angle(bw_outbuf, width, height, PROFONDEUR);
            angle = apm[0];
            p = apm[1];
            m = apm[2];
            free(apm);
            double* apm_aplati = aplatir(angle, p, m, PROFONDEUR);
            angle_aplati = apm_aplati[0];
            p_aplati = apm_aplati[1];
            m_aplati = apm_aplati[2];
            free(apm_aplati);
            angle = PI*angle/180;
            angle_aplati = PI*angle_aplati/180;
            angle_utilise = angle_aplati;
            
            if (mode_P) {
                T = 1.0f / (P * (pourcentage_Vmax * Vmax / 100.0f)); // en secondes
            }
            int* vitesses = get_vitesse_mot(Vmax * pourcentage_Vmax / 100.0, angle_utilise, T); // en rpm
            v_mot_droit = vitesses[0];
            v_mot_gauche = vitesses[1];
            printf("Angle: %.2f rad, V droite: %d rpm, V gauche: %d rpm\n",
                angle_utilise, v_mot_droit, v_mot_gauche);
            
            if (!pause) {
                motor_define_direction_from_pwm(v_mot_droit, v_mot_gauche);
                motor_set_rpm(&moteur0, v_mot_droit*signe(v_mot_droit));
                motor_set_rpm(&moteur1, v_mot_gauche*signe(v_mot_gauche)); // *signe pour avoir la valeur absolue
            }
            else {
                motor_set_rpm(&moteur0, 0.0);
                motor_set_rpm(&moteur1, 0.0);
            }
        }

        
        printf("Core 0: Processing time (us): %llu\n", time_us_64() - t_us_core_0_beginning_loop);

        // FPS max → pas de pause
        tight_loop_contents();
    }

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
    stdio_init_all();
    sleep_ms(500);
    coded_image = (uint8_t*)malloc(1024*sizeof(uint8_t)); // Taille max possible
    mutex_init(&multicore_lock);
    multicore_launch_core1(core1_entry);
    core0_entry();
    return 0;
}



