#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lib.h"
#include "hardware/i2c.h"
#include "pico/multicore.h"
#include "pico/sync.h"

#define Vmax 70 // RPM max des moteurs
#define SENSIBILITE 5

static char  str_v_mot[32];
static uint64_t t_us_core_0_beginning_loop;
static uint64_t t_us_core_1_beginning_loop;
int v_mot_droit;
int v_mot_gauche;
double angle;*
uint32_t debut = 0; // Pour gérer le timer de recherche de ligne
bool LIGNE_DETECTEE = true; // Indique si la ligne est détectée ou non


static uint8_t* outbuf1;
static uint8_t* outbuf2;
static bool toggle_buf = false;

static mutex_t multicore_lock;

static uint8_t** get_outbuf_from_core(bool core) {
    // mutex_enter_blocking(&multicore_lock);
    // printf("Core %d: Getting output buffer (toggle_buf=%d)\n", core, toggle_buf);
    // uint64_t t_us = time_us_64();
    // printf("Core %d: time (us): %llu\n", core, t_us);

    uint8_t** outbuf = (toggle_buf == core) ? &outbuf1 : &outbuf2;
    // mutex_exit(&multicore_lock);
    return outbuf;
}

static bool core_0_ready = false;
static bool core_1_ready = false;
static void core_ready_to_swap(bool core, bool ready)
{
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

void interpretCommand(TCP_SERVER_T *state, const char* command)
{
    // Implémentez ici l'interprétation des commandes reçues
    command = trim_whitespace_divers((char *)command);
    printf("Interpreted Command: '%s'\n", command);
    if (strcmp(command, "LED ON") == 0)
    {
        pico_set_led(true);
        // tcp_server_send(state, "LED is ON", PACKET_TYPE_GENERAL);
    }
    else if (strcmp(command, "LED OFF") == 0)
    {
        pico_set_led(false);
        // tcp_server_send(state, "LED is OFF", PACKET_TYPE_GENERAL);
    }
    // else if (estNombreEntier(command))
    // {
    //     char result[100]; // Buffer statique
    //     // snprintf(result, sizeof(result), "%s%s", prefix, command);
    //     tcp_server_send(state, command, PACKET_TYPE_MOT_0);
    // }
    // else
    // {
    //     // Réponse simple : renvoyer exactement ce qu'on a reçu
    //     tcp_server_send(state, command, PACKET_TYPE_GENERAL);
    // }
}

void core1_entry()
{
    // Initialisation Wi-Fi
    err_t connect_success = wifi_auto_connect();
    if (connect_success != ERR_OK)
    {
        printf("Échec de la connexion Wi-Fi.\n");
        pico_blink_led(10, 100); // Clignote rapidement pour indiquer l'erreur
        pico_set_led(false); 
        // return 1;
    } else pico_set_led(true);

    TCP_SERVER_T* state;
    uint8_t rx_buffer[BUF_SIZE];
    if (connect_success == ERR_OK) state = tcp_server_start();

    // Initialisation OLED
    initOLED(I2C_SDA, I2C_SCL);
    renderSymbol(RPI,1,56); // Render the Rpi Symbol
    renderIPString(IP4ADDR);
    if (connect_success == ERR_OK)
        {
        while (true)
        {
                t_us_core_1_beginning_loop = time_us_64();

                int received = tcp_server_receive(state, rx_buffer, BUF_SIZE);

                if (received > 0)
                {
                    // Ajouter un '\0' pour créer une chaîne C
                    rx_buffer[received] = '\0';

                    printf("Reçu du client : %s\n", rx_buffer);

                    // Réponse simple : renvoyer exactement ce qu'on a reçu
                    interpretCommand(state, (const char *)rx_buffer);
                }
                sleep_ms(10);

                snprintf(str_v_mot, sizeof(str_v_mot), "%d", v_mot_droit);
                tcp_server_send(state, str_v_mot, PACKET_TYPE_MOT_0);
                snprintf(str_v_mot, sizeof(str_v_mot), "%d", v_mot_gauche);
                tcp_server_send(state, str_v_mot, PACKET_TYPE_MOT_1);
                snprintf(str_v_mot, sizeof(str_v_mot), "%.6f", 180*angle/PI);
                tcp_server_send(state, str_v_mot, PACKET_TYPE_GENERAL);

                err_t err = tcp_send_large_img(state, *get_outbuf_from_core(1), MAX_WIDTH*MAX_HEIGHT);
                printf("TCP send time (us): %llu\n", time_us_64() - t_us_core_1_beginning_loop);
                if (err != ERR_OK)
                {
                    printf("Erreur d'envoi de l'image : %d\n", err);
                }
                uint8_t** pointer_to_outbuf = get_outbuf_from_core(1);
                core_ready_to_swap(1, true);
                printf("Core 1: Waiting for swap...\n");
                while (pointer_to_outbuf == get_outbuf_from_core(1))
                {
                    sleep_ms(10);
                    // tight_loop_contents();
                }
                printf("Core 1: Swap done\n");
                printf("Core 1: Processing time (us): %llu\n", time_us_64() - t_us_core_1_beginning_loop);

            tight_loop_contents();
        }
    }
}


void core0_entry()
{
    // Initialisation moteurs
    printf("Initialisation des moteurs\n");
    init_motor_and_encoder(&moteur0);
    init_motor_and_encoder(&moteur1);
    motor_set_direction(&moteur0, 1);
    motor_set_direction(&moteur1, 0);
    motor_set_pwm(&moteur0, 0.);
    motor_set_pwm(&moteur1, 0.);

    /* INITIALISATION CAMERA */
    struct camera camera;
    init_camera();
    struct camera_platform_config platform = create_camera_platform_config();

    /* Choix Format */
    uint16_t width_temp, height_temp;
    OV7670_size size;
    int division = 0; // 0 : DIV 8, 1 : DIV 4, ...
    int format_out = choix_format(division, &width_temp, &height_temp, &size);
    const uint16_t width = width_temp;
    const uint16_t height = height_temp;

    printf("Camera to be initialised\n");
    if (camera_init(&camera, &platform, size)) printf("Erreur d'initialisation de la caméra\n"); // return 1;
    printf("Camera initialised\n");
    motor_set_pwm(&moteur1, 0.);

    /* Creation Buffers Camera */
    static uint8_t *frame_buffer, *bw_outbuf;
    creation_buffers_camera(&frame_buffer, get_outbuf_from_core(0),
                           &bw_outbuf, width, height);

    creation_buffers_camera(&frame_buffer, get_outbuf_from_core(1),
                           &bw_outbuf, width, height);

    err_t err;
    
    while (true) {
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
        #endif
        
        uint8_t** pointer_to_outbuf = get_outbuf_from_core(0);
        t_us_core_0_beginning_loop = time_us_64();
        core_ready_to_swap(0, false);
        camera_capture_blocking(&camera, frame_buffer, width, height);
        // printf("Capture time (us): %llu\n", time_us_64() - t_us_core_0_beginning_loop);
        // Header P5 : format et dimensions de l'image
        // printf("P5\n%d %d\n255\n", width, height);

        // Extraire Y seulement

        for (int px = 0; px < width * height; px++)
            (*get_outbuf_from_core(0))[px] = frame_buffer[px * 2]; 

        // Traitement
        int seuillage_out = seuillage(*pointer_to_outbuf, bw_outbuf,
                                      width, height);
        core_ready_to_swap(0, true);

        if (ligne_detectee(bw_outbuf, width, height) == 0)
        {
            // Ligne non détectée
            if (!LIGNE_DETECTEE)
            {
                // Déjà en mode recherche de ligne
                uint32_t maintenant = time_us_64();
                uint32_t time = maintenant / 1000 - debut / 1000;
                chercher_ligne(time);
            }
            else
            {
                // Passage en mode recherche de ligne
                debut = time_us_64();
                chercher_ligne(0);
            }
            LIGNE_DETECTEE = false
        }
        else
        {
            LIGNE_DETECTEE = true
            /* Version sinus */
            angle = PI*trouver_angle(bw_outbuf, width, height)/180;
            // v_mot_droit = (Vmax/2)*(1+sin(angle));
            // v_mot_gauche = (Vmax/2)*(1-sin(angle));
            double projection_inter = 1/(SENSIBILITE*tan(angle))*(1/(SENSIBILITE*tan(angle)));
            printf("Projection inter: %.3f\n", projection_inter);
            double projection = signe(angle)*sqrt(1/(1+projection_inter));
            printf("Projection: %.3f\n", projection);
            v_mot_droit = (Vmax/2)*(1+projection);
            v_mot_gauche = (Vmax/2)*(1-projection);

            // v_mot_droit = Vmax;
            // v_mot_gauche = Vmax;

            /* Version lineaire */
            // double angle = trouver_angle(bw_outbuf, width, height);
            // int v_mot_droit = Vmax/2*(1+angle/90);
            // int v_mot_gauche = Vmax/2*(1-angle/90);

            printf("Angle: %.3f radians, Vitesse Moteur Droit: %d RPM, Vitesse Moteur Gauche: %d RPM\n",
                angle, v_mot_droit, v_mot_gauche);
        }
        motor_set_pwm_brut(&moteur0, pwm_lookup_for_rpm(v_mot_droit));
        motor_set_pwm_brut(&moteur1, pwm_lookup_for_rpm(v_mot_gauche));
        printf("PWM Moteur Droit: %d, PWM Moteur Gauche: %d\n",
               pwm_lookup_for_rpm(v_mot_droit), pwm_lookup_for_rpm(v_mot_gauche));
        // printf("PWM Moteur Droit: %d, PWM Moteur Gauche: %d\n",
            //    pwm_lookup_for_rpm(v_mot_droit), pwm_lookup_for_rpm(v_mot_gauche));
        // motor_set_pwm(&moteur0, 50+v_mot_droit/2);
        // motor_set_pwm(&moteur1, 50+v_mot_gauche/2);
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
    sleep_ms(2000);
    mutex_init(&multicore_lock);
    multicore_launch_core1(core1_entry);
    core0_entry();
    return 0;
}



