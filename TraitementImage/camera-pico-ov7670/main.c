#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "camera.h"
#include "init_camera.h"
#include "traitement.h"


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
        int direction = choix_direction(bw_outbuf, width, height);
        // Envoi de l'image
        fwrite(outbuf, 1, width * height, stdout);
        fflush(stdout);

        // FPS max → pas de pause
        tight_loop_contents();
    }

    free(frame_buffer);
    free(outbuf);
    return 0;
}
