#ifndef __INIT_CAMERA_H__
#define __INIT_CAMERA_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "../include/camera.h"

#define I2C_SDA 26
#define I2C_SCL 27

struct camera_platform_config create_camera_platform_config(void);

int init_camera();

int choix_format(int division,
    uint16_t* width, uint16_t* height,
    OV7670_size* size);

int creation_buffers_camera(uint8_t **frame_buffer, uint8_t **outbuf, uint8_t **bw_outbuf,
                            uint16_t width, uint16_t height);

#endif