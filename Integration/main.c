#include <stdio.h>
#include "pico/stdlib.h"
#include "lib/lib.h"
#include "hardware/i2c.h"

#define I2C_SDA 26
#define I2C_SCL 27


// Supprime les espaces en début et fin de chaîne
char* trim_whitespace(char *line) {
    char *s = line;
    while (*s && (*s == ' ' || *s == '\t')) s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\0' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    return s;
}

int estNombreEntier(const char *s) {
    char *end;
    long val = strtol(s, &end, 10);
    // Vérifie si la conversion a consommé toute la chaîne et qu'il n'y a pas eu d'erreur
    return (*end == '\0' && s != end);
}

void interpretCommand(TCP_SERVER_T *state, const char* command) {
    // Implémentez ici l'interprétation des commandes reçues
    command = trim_whitespace((char *)command);
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
        pico_blink_led(10, 200); // Clignote rapidement pour indiquer l'erreur
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
    sleep_ms(2000);
    motor_set_pwm(&moteur0, 100.);
    motor_set_pwm(&moteur1, 100.);
    sleep_ms(2000);
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
    uint8_t *frame_buffer, *outbuf, *bw_outbuf;
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
        
        // uint64_t t_us_current = time_us_64();
        // if (t_us_current - t_us_previous >= 1000000) { // 1 second
        //     t_us_previous = t_us_current;
        //     pico_toggle_led();
        //     // printf("hello\n");
        //     tcp_server_send(state, (const uint8_t *)"Hello, client!", 14);
        // }
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
        // printf("Seuilage time (us): %llu\n", time_us_64() - t_us_current);
        int direction = choix_direction(bw_outbuf, width, height);
        // Envoi de l'image
        // fwrite(outbuf, 1, width * height, stdout);
        // fflush(stdout);
        t_us_current = time_us_64();
        if (direction == 1) {
            // interpretCommand(state, "TURN LEFT");
            motor_set_pwm(&moteur0, 10.);
            motor_set_pwm(&moteur1, 0.);
            if (connect_success == ERR_OK) {
                tcp_server_send(state, "10", PACKET_TYPE_MOT_0);
                tcp_server_send(state, "0", PACKET_TYPE_MOT_1);
            }
        } else if (direction == -1) {
            motor_set_pwm(&moteur1, 10.);
            motor_set_pwm(&moteur0, 0.);
            if (connect_success == ERR_OK) {
                tcp_server_send(state, "0", PACKET_TYPE_MOT_0);
                tcp_server_send(state, "10", PACKET_TYPE_MOT_1);
            }
        } else {
            // interpretCommand(state, "FORWARD");
            motor_set_pwm(&moteur0, 10.);
            motor_set_pwm(&moteur1, 10.);
            if (connect_success == ERR_OK) {
                tcp_server_send(state, "10", PACKET_TYPE_MOT_0);
                tcp_server_send(state, "10", PACKET_TYPE_MOT_1);
            }
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



