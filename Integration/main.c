#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lib.h"
#include "hardware/i2c.h"

#define I2C_SDA 26
#define I2C_SCL 27

#define Vmax 40

static char  str_v_mot[32];




void interpretCommand(TCP_SERVER_T *state, const char* command) {
    // Implémentez ici l'interprétation des commandes reçues
    command = trim_whitespace_divers((char *)command);
    printf("Interpreted Command: '%s'\n", command);
    if (strcmp(command, "LED ON") == 0) {
        pico_set_led(true);
        tcp_server_send(state, "LED is ON", PACKET_TYPE_GENERAL);
    } else if (strcmp(command, "LED OFF") == 0) {
        pico_set_led(false);
        tcp_server_send(state, "LED is OFF", PACKET_TYPE_GENERAL);
    } else if (estNombreEntier(command)) {
        char result[100]; // Buffer statique
        // snprintf(result, sizeof(result), "%s%s", prefix, command);
        tcp_server_send(state, command, PACKET_TYPE_MOT_0);
    } else {
        // Réponse simple : renvoyer exactement ce qu'on a reçu
        tcp_server_send(state, command, PACKET_TYPE_GENERAL);
    }
}


// BUFF_SIZE défini dans tcp_server.h

int main() {
    stdio_init_all();
    sleep_ms(2000);
    // Initialisation Wi-Fi
    err_t connect_success = wifi_auto_connect();
    if (connect_success != ERR_OK) {
        printf("Échec de la connexion Wi-Fi.\n");
        pico_blink_led(10, 100); // Clignote rapidement pour indiquer l'erreur
        pico_set_led(false); 
        // return 1;
    } else pico_set_led(true); 
    
    TCP_SERVER_T* state;
    if (connect_success == ERR_OK) state = tcp_server_start();

    // Initialisation OLED
    initOLED(I2C_SDA, I2C_SCL);
    renderSymbol(RPI,1,56); // Render the Rpi Symbol
    renderIPString(IP4ADDR);

    uint8_t rx_buffer[BUF_SIZE];

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
    if (camera_init(&camera, &platform, size)) return 1;
    printf("Camera initialised\n");
    motor_set_pwm(&moteur1, 0.);
    /* Creation Buffers Camera */
    static uint8_t *frame_buffer, *outbuf, *bw_outbuf;
    int creation_buffer_out = creation_buffers_camera(&frame_buffer, &outbuf,
                           &bw_outbuf, width, height);


    err_t err;
    while (true) {
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
        #endif
        if (connect_success == ERR_OK) {
            int received = tcp_server_receive(state, rx_buffer, BUF_SIZE);

            if (received > 0) {
                // Ajouter un '\0' pour créer une chaîne C
                rx_buffer[received] = '\0';

                printf("Reçu du client : %s\n", rx_buffer);

                // Réponse simple : renvoyer exactement ce qu'on a reçu
                interpretCommand(state, (const char *)rx_buffer);
            }
            sleep_ms(100);
        }
        
        uint64_t t_us_current = time_us_64();
        camera_capture_blocking(&camera, frame_buffer, width, height);
        printf("Capture time (us): %llu\n", time_us_64() - t_us_current);
        // Header P5 : format et dimensions de l'image
        // printf("P5\n%d %d\n255\n", width, height);

        // Extraire Y seulement
        for (int px = 0; px < width * height; px++)
            outbuf[px] = frame_buffer[px * 2];

        // Traitement
        t_us_current = time_us_64();
        int seuillage_out = seuillage(outbuf, bw_outbuf,
                                      width, height);

        double angle = PI*trouver_angle(bw_outbuf, width, height)/180;
        int v_mot_droit = (Vmax/2)*(1+sin(angle));
        int v_mot_gauche = (Vmax/2)*(1-sin(angle));

        // double angle = trouver_angle(bw_outbuf, width, height);
        // int v_mot_droit = Vmax/2*(1+angle/90);
        // int v_mot_gauche = Vmax/2*(1-angle/90);

        motor_set_pwm(&moteur0, 50+v_mot_droit/2);
        motor_set_pwm(&moteur1, 50+v_mot_gauche/2);
        if (connect_success == ERR_OK) {
            snprintf(str_v_mot, sizeof(str_v_mot), "%d", v_mot_droit);
            tcp_server_send(state, str_v_mot, PACKET_TYPE_MOT_0);
            snprintf(str_v_mot, sizeof(str_v_mot), "%d", v_mot_gauche);
            tcp_server_send(state, str_v_mot, PACKET_TYPE_MOT_1);
            snprintf(str_v_mot, sizeof(str_v_mot), "%.6f", 180*angle/PI);
            tcp_server_send(state, str_v_mot, PACKET_TYPE_GENERAL);
        }


        if (connect_success == ERR_OK) {
            err = tcp_send_large_img(state, outbuf, width*height);
            printf("TCP send time (us): %llu\n", time_us_64() - t_us_current);
            if (err != ERR_OK) {
                printf("Erreur d'envoi de l'image : %d\n", err);
            }
        }
        // FPS max → pas de pause
        tight_loop_contents();
    }

    // tcp_server_stop(state);
    pico_set_led(false);
    cyw43_arch_deinit();
    free(frame_buffer);
    free(outbuf);
    free(bw_outbuf);
    return 0;
}



