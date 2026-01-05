#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../include/init_camera.h"
#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "../include/camera.h"

int __i2c_write_blocking(void *i2c_handle, uint8_t addr,
    const uint8_t *src, size_t len)
{
    return i2c_write_blocking((i2c_inst_t *)i2c_handle, addr, src, len, false);
}

int __i2c_read_blocking(void *i2c_handle, uint8_t addr,
    uint8_t *dst, size_t len)
{
    return i2c_read_blocking((i2c_inst_t *)i2c_handle, addr, dst, len, false);
}


struct camera_platform_config create_camera_platform_config()
{
    struct camera_platform_config platform = {
        .i2c_write_blocking = __i2c_write_blocking,
        .i2c_read_blocking = __i2c_read_blocking,
        .i2c_handle = i2c1,
        .xclk_pin = CAMERA_XCLK_PIN,
        .xclk_divider = 15,  // 5 MHz, stable
        .vsync_pin = CAMERA_VSYNC_PIN,
        .href_pin = CAMERA_HREF_PIN,
        .pclk_pin = CAMERA_PCLK_PIN,
        .data_pins = {
            CAMERA_D0, CAMERA_D1, CAMERA_D2, CAMERA_D3,
            CAMERA_D4, CAMERA_D5, CAMERA_D6, CAMERA_D7,
        },
    };
    return platform;
}


int init_camera()
{
    // init I2C:
    i2c_init(i2c1, 100000);
    gpio_set_function(CAMERA_SDA, GPIO_FUNC_I2C);
    gpio_set_function(CAMERA_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(CAMERA_SDA);
    gpio_pull_up(CAMERA_SCL);

    // init RES:
    gpio_init(CAMERA_RES_PIN);
    gpio_set_dir(CAMERA_RES_PIN, GPIO_OUT);
    gpio_put(CAMERA_RES_PIN, true);

    // init PWDN:
    gpio_init(CAMERA_PWDN_PIN);
    gpio_set_dir(CAMERA_PWDN_PIN, GPIO_OUT);
    gpio_put(CAMERA_PWDN_PIN, false);

    // init sync pins
    gpio_init(CAMERA_VSYNC_PIN);
    gpio_set_dir(CAMERA_VSYNC_PIN, GPIO_IN);
    gpio_init(CAMERA_HREF_PIN);
    gpio_set_dir(CAMERA_HREF_PIN, GPIO_IN);
    gpio_init(CAMERA_PCLK_PIN);
    gpio_set_dir(CAMERA_PCLK_PIN, GPIO_IN);

    // TODO : essayer bus aquisition parallele
    // init DATA pins
    int datapins[8] = {CAMERA_D0, CAMERA_D1, CAMERA_D2, CAMERA_D3,
        CAMERA_D4, CAMERA_D5, CAMERA_D6, CAMERA_D7};
    for (int i=0; i<8; i++) {
        gpio_init(datapins[i]);
        gpio_set_dir(datapins[i], GPIO_IN);
    }
    return 0;
}

int choix_format(int division,
    uint16_t* width, uint16_t* height,
    OV7670_size* size)
{
    const uint16_t tableau_width[] = {CAMERA_WIDTH_DIV8,
        CAMERA_WIDTH_DIV4, CAMERA_WIDTH_DIV2, CAMERA_WIDTH_DIV1};
    const uint16_t tableau_height[] = {CAMERA_HEIGHT_DIV8,
        CAMERA_HEIGHT_DIV4, CAMERA_HEIGHT_DIV2, CAMERA_HEIGHT_DIV1};
    OV7670_size tableau_size[] = {OV7670_SIZE_DIV8, ///< 640 x 480
                                  OV7670_SIZE_DIV4, ///< 320 x 240
                                  OV7670_SIZE_DIV2, ///< 160 x 120
                                  OV7670_SIZE_DIV1, ///< 80 x 60
                                  };
    if (division < 0 || division >= 5)
        return 1;
    *width = tableau_width[division];
    *height = tableau_height[division];
    *size = tableau_size[division];
    return 0;
}

int creation_buffers_camera(uint8_t *frame_buffer[2], uint8_t *outbuf[2], uint8_t *bw_outbuf[2],
                            uint16_t width, uint16_t height)
{
    for (int coeur = 0 ; coeur < 2 ; coeur++)
    {
        frame_buffer[coeur] = malloc(2 * width * height);
        if (!frame_buffer[coeur]) return 1;

        outbuf[coeur] = malloc(width * height);   // 1 octet par pixel (P5)
        if (!outbuf[coeur]) {
            free(frame_buffer[coeur]);
            return 1;
        }

        bw_outbuf[coeur] = malloc(width * height);   // 1 octet par pixel (P5) (1 bit suffirait)
        if (!bw_outbuf[coeur]) {
            free(frame_buffer[coeur]);
            free(outbuf[coeur]);
            return 1;
        }
    }
    return 0;
}









