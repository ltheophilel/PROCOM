#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "lib/lib.h"
#include "hardware/i2c.h"

#define I2C_SDA 26
#define I2C_SCL 27

#define MSG_PROCESSED_FLAG 0x80000000u

static uint8_t *frame_buffer[2] = {NULL, NULL}; // taille: WIDTH * HEIGHT * 2
static uint8_t *outbuf[2] = {NULL, NULL}; // taille: WIDTH * HEIGHT
static uint8_t *bw_outbuf[2] = {NULL, NULL}; // taille: WIDTH * HEIGHT

static uint16_t width, height;

static TCP_SERVER_T* state;
static uint64_t t_us_current;
static err_t err;

struct camera camera;

void core1_entry(void);


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

/*
===============================================================
                             CORE 1
===============================================================
*/
void core1_entry()
{
    while (true)
    {
        uint32_t msg = multicore_fifo_pop_blocking();
        if (msg & MSG_PROCESSED_FLAG)
        {
            continue;
            // message de confirmation (non attendu ici) — on l'ignore
        }

        uint32_t current = msg & 1u; // choix du buffer

        // recopie des pointeurs buffers
        uint8_t *frame_buffer_c1 = frame_buffer[current];
        uint8_t *outbuf_c1 = outbuf[current];
        uint8_t *bw_outbuf_c1 = bw_outbuf[current];

        if (!frame_buffer_c1 || !outbuf_c1 || !bw_outbuf_c1)
        {
            // si quelque chose s'est mal passé, on signale une erreur
            multicore_fifo_push_blocking(MSG_PROCESSED_FLAG | current);
            continue;
        }
        // t_us_current = time_us_64();

        camera_capture_blocking(&camera, frame_buffer[current], width, height);
        // printf("Capture time (us): %llu\n", time_us_64() - t_us_current);

        // Extraire Y seulement
        for (int px = 0; px < width * height; px++)
            outbuf_c1[px] = frame_buffer_c1[px * 2];

        // Traitement
        // t_us_current = time_us_64();
        int seuillage_out = seuillage(outbuf_c1, bw_outbuf_c1,
                                      width, height);
        // printf("Seuillage time (us): %llu\n", time_us_64() - t_us_current);
        int direction = choix_direction(bw_outbuf_c1, width, height);

        multicore_fifo_push_blocking(MSG_PROCESSED_FLAG | current);
        // "J'ai fini"
    }
}

// BUFF_SIZE défini dans tcp_server.h
/*
===============================================================
                             CORE 0
===============================================================
*/
int main() {
    stdio_init_all();
    sleep_ms(2000);
    // Initialisation Wi-Fi
    err_t connect_success = wifi_auto_connect();
    if (connect_success != ERR_OK) {
        printf("Échec de la connexion Wi-Fi. Arrêt du programme.\n");
        pico_blink_led(10, 200); // Clignote rapidement pour indiquer l'erreur
        return 1;
    }
    pico_set_led(true);

    state = tcp_server_start();


    // Initialisation OLED
    initOLED(I2C_SDA, I2C_SCL);
    renderSymbol(RPI,1,56); // Render the Rpi Symbol
    renderIPString(IP4ADDR);

    uint8_t rx_buffer[BUF_SIZE];

    /* INITIALISATION CAMERA */

    init_camera();
    struct camera_platform_config platform = create_camera_platform_config();

    /* Choix Format */
    uint16_t width_temp, height_temp;
    OV7670_size size;
    int division = 0; // 0 : DIV 8, 1 : DIV 4, ...
    int format_out = choix_format(division, &width_temp, &height_temp, &size);
    width = width_temp;
    height = height_temp;

    printf("Camera to be initialised\n");
    if (camera_init(&camera, &platform, size)) return 1;
    printf("Camera initialised\n");

    /* Creation Buffers Camera */
    int creation_buffer_out = creation_buffers_camera(frame_buffer, outbuf,
                           bw_outbuf, width, height);

    // Lancement du 2e coeur
    multicore_launch_core1(core1_entry);
    bool use_ping = 1;
    static bool first = true;
    uint32_t current = 0;
    uint32_t previous = 1;

    while (true)
    {
        #if PICO_CYW43_ARCH_POLL
            cyw43_arch_poll();
        #endif
        int received = tcp_server_receive(state, rx_buffer, BUF_SIZE);

        if (received > 0) {
            // Ajouter un '\0' pour créer une chaîne C
            rx_buffer[received] = '\0';

            printf("Reçu du client : %s\n", rx_buffer);

            // Réponse simple : renvoyer exactement ce qu'on a reçu
            interpretCommand(state, (const char *)rx_buffer);
        }
        sleep_ms(100);
        
        // uint64_t t_us_current = time_us_64();
        // if (t_us_current - t_us_previous >= 1000000) { // 1 second
        //     t_us_previous = t_us_current;
        //     pico_toggle_led();
        //     // printf("hello\n");
        //     tcp_server_send(state, (const uint8_t *)"Hello, client!", 14);
        // }

        multicore_fifo_push_blocking(current); // Debut capture et traitement

        // Header P5 : format et dimensions de l'image
        // fwrite(outbuf, 1, width * height, stdout);
        // fflush(stdout);
        // printf("P5\n%d %d\n255\n", width, height);

        /*     Envoi de l'image     */

        if (!first)
        {
            err = tcp_send_large_img(state, outbuf[previous], width*height);
            // printf("TCP send time (us): %llu\n", time_us_64() - t_us_current);
            if (err != ERR_OK)
            {
                printf("Erreur d'envoi de l'image : %d\n", err);
            }
        }

        uint32_t ack = multicore_fifo_pop_blocking();
        previous = current;
        current ^= 1;
        first = false; // On interchange les 2 buffers

        t_us_current = time_us_64();

        // FPS max → pas de pause
        tight_loop_contents();
    }
    // Jamais atteint
    // tcp_server_stop(state);
    pico_set_led(false);
    cyw43_arch_deinit();
    return 0;
}