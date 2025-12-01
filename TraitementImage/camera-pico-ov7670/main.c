#include <stdlib.h>
#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "camera.h"

#define CAMERA_XCLK_PIN 21
#define CAMERA_PCLK_PIN 10
#define CAMERA_HREF_PIN 9
#define CAMERA_RES_PIN 16
#define CAMERA_PWDN_PIN 15
#define CAMERA_VSYNC_PIN 22
#define CAMERA_SDA      26
#define CAMERA_SCL      27
#define CAMERA_D0 14
#define CAMERA_D1 17
#define CAMERA_D2 13
#define CAMERA_D3 18
#define CAMERA_D4 12
#define CAMERA_D5 19
#define CAMERA_D6 11
#define CAMERA_D7 20

static inline int __i2c_write_blocking(void *i2c_handle, uint8_t addr, const uint8_t *src, size_t len)
{
    return i2c_write_blocking((i2c_inst_t *)i2c_handle, addr, src, len, false);
}

static inline int __i2c_read_blocking(void *i2c_handle, uint8_t addr, uint8_t *dst, size_t len)
{
    return i2c_read_blocking((i2c_inst_t *)i2c_handle, addr, dst, len, false);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

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
    int datapins[8] = {CAMERA_D0, CAMERA_D1, CAMERA_D2, CAMERA_D3, CAMERA_D4, CAMERA_D5, CAMERA_D6, CAMERA_D7};
    for (int i=0; i<8; i++) {
        gpio_init(datapins[i]);
        gpio_set_dir(datapins[i], GPIO_IN);
    }

    // init camera:
    struct camera camera;
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

    const uint16_t width = CAMERA_WIDTH_DIV8;
    const uint16_t height = CAMERA_HEIGHT_DIV8;

    printf("Camera to be initialized\n");
    if (camera_init(&camera, &platform)) return 1;
    printf("Camera initialized\n");

    uint8_t *frame_buffer = malloc(2 * width * height);
    if (!frame_buffer) return 1;

    uint8_t *outbuf = malloc(width * height);   // 1 octet par pixel (P5)
    if (!outbuf) {
        free(frame_buffer);
        return 1;
    }

    while (1) {

        camera_capture_blocking(&camera, frame_buffer);

        // Header P5
        printf("P5\n%d %d\n255\n", width, height);

        // Extraire Y seulement
        for (int px = 0; px < width * height; px++)
            outbuf[px] = frame_buffer[px * 2];

        fwrite(outbuf, 1, width * height, stdout);
        fflush(stdout);

        // FPS max → pas de pause
        tight_loop_contents();
    }

    free(frame_buffer);
    free(outbuf);
    return 0;
}
