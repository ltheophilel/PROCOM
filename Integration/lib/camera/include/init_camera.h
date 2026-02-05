#ifndef __INIT_CAMERA_H__
#define __INIT_CAMERA_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "camera.h"

/* // BRANCHEMENTS :
// ! GP pins
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
#define CAMERA_D7 20 */

// BRANCHEMENTS : 
// ! GP pins
#define CAMERA_HREF_PIN 9
#define CAMERA_PCLK_PIN 10
#define CAMERA_PWDN_PIN 11
#define CAMERA_RES_PIN 12
#define CAMERA_XCLK_PIN 13
#define CAMERA_D0 14
#define CAMERA_D1 15
#define CAMERA_D2 16
#define CAMERA_D3 17
#define CAMERA_D4 18
#define CAMERA_D5 19
#define CAMERA_D6 20
#define CAMERA_D7 21
#define CAMERA_VSYNC_PIN 22
#define CAMERA_SDA      26
#define CAMERA_SCL      27

#define I2C_SDA 26
#define I2C_SCL 27

#define FRAME_SIZE (CAMERA_WIDTH_DIV8 * CAMERA_HEIGHT_DIV8 * 2)

#define CAMERA_WIDTH_DIV8  80
#define CAMERA_HEIGHT_DIV8 60

#define CAMERA_WIDTH_DIV4  160
#define CAMERA_HEIGHT_DIV4 120

#define CAMERA_WIDTH_DIV2  320
#define CAMERA_HEIGHT_DIV2 240

#define CAMERA_WIDTH_DIV1  640
#define CAMERA_HEIGHT_DIV1 480

struct camera_platform_config create_camera_platform_config(void);

int init_camera();

int choix_format(int division,
    uint16_t* width, uint16_t* height,
    OV7670_size* size);

int creation_buffers_camera(uint8_t **frame_buffer, uint8_t **outbuf, uint8_t **bw_outbuf,
                            uint16_t width, uint16_t height);

#endif