#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "../include/camera.h"
#include "../include/init_camera.h"
#include "../include/traitement.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "ov7670_capture.pio.h"


int main()
{
    stdio_init_all();
    sleep_ms(2000);

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
    camera_pio_init();

    /* Creation Buffers Camera */
    uint8_t *frame_buffer, *outbuf, *bw_outbuf;
    int creation_buffer_out = creation_buffers_camera(&frame_buffer, &outbuf,
                           &bw_outbuf, width, height);

    while (1)
    {
        camera_capture_blocking(&camera, frame_buffer, width, height);

        // Header P5 : format et dimensions de l'image
        printf("P5\n%d %d\n255\n", width, height);

        // Extraire Y seulement
        for (int px = 0; px < width * height; px++)
            outbuf[px] = frame_buffer[px * 2];

        // Traitement
        int seuillage_out = seuillage(outbuf, bw_outbuf,
                        width, height);
        double angle = trouver_angle(bw_outbuf, width, height);

        double command = GAIN_REGLAGE * angle;

        //if (command > max_command) command = max_command;
        //if (command < -max_command) command = -max_command;

        // Affichage de l'angle et de la direction
        // printf("Commande = %.3f\n", command);
        // printf("Direction = ");

        // if (command < -1) {
        //     printf("Gauche\n");
        // } else if (command > 1) {
        //     printf("Droite\n");
        // } else {
        //     printf("Tout droit\n");
        // }

        // Envoi de l'image
        fwrite(outbuf, 1, width * height, stdout);
        fflush(stdout);
        // FPS max → pas de pause
        tight_loop_contents();
    }

    free(frame_buffer);
    free(outbuf);
    free(bw_outbuf);
    return 0;
}
