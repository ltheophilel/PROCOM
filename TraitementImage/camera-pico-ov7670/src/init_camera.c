#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../include/init_camera.h"
#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "../include/camera.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "ov7670_capture.pio.h"

PIO camera_pio = pio0;
uint camera_sm = 0;
int camera_dma_chan;

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

int creation_buffers_camera(uint8_t **frame_buffer, uint8_t **outbuf, uint8_t **bw_outbuf,
                            uint16_t width, uint16_t height)
{
    *frame_buffer = malloc(2 * width * height);
    if (!frame_buffer) return 1;

    *outbuf = malloc(width * height);   // 1 octet par pixel (P5)
    if (!outbuf) {
        free(frame_buffer);
        return 1;
    }

    *bw_outbuf = malloc(width * height);   // 1 octet par pixel (P5) (1 bit suffirait)
    if (!bw_outbuf) {
        free(frame_buffer);
        free(outbuf);
        return 1;
    }
    return 0;
}

void camera_pio_init(void)
{
    uint offset = pio_add_program(camera_pio, &ov7670_capture_program);

    pio_sm_config c =
        ov7670_capture_program_get_default_config(offset);

    sm_config_set_in_pins(&c, CAMERA_D0);
    // sm_config_set_wait_pin(&c, CAMERA_PCLK_PIN);
    sm_config_set_jmp_pin(&c, CAMERA_HREF_PIN);

    sm_config_set_in_shift(&c, false, true, 8);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    for (int i = 0; i < 8; i++)
        pio_gpio_init(camera_pio, CAMERA_D0 + i);

    pio_gpio_init(camera_pio, CAMERA_PCLK_PIN);
    pio_gpio_init(camera_pio, CAMERA_HREF_PIN);
    pio_gpio_init(camera_pio, CAMERA_VSYNC_PIN);

    pio_sm_set_consecutive_pindirs(
        camera_pio, camera_sm, CAMERA_D0, 8, false);

    pio_sm_init(camera_pio, camera_sm, offset, &c);
    pio_sm_set_enabled(camera_pio, camera_sm, true);

    camera_dma_chan = dma_claim_unused_channel(true);
}
