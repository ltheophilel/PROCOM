/**
 * Copyright (c) 2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <stdint.h>
#include "ov7670.h"
#include <stdio.h>
#include "pico/cyw43_arch.h"

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

#define FRAME_SIZE (CAMERA_WIDTH_DIV8 * CAMERA_HEIGHT_DIV8 * 2)

#define CAMERA_WIDTH_DIV8  80
#define CAMERA_HEIGHT_DIV8 60

#define CAMERA_WIDTH_DIV4  160
#define CAMERA_HEIGHT_DIV4 120

#define CAMERA_WIDTH_DIV2  320
#define CAMERA_HEIGHT_DIV2 240

#define CAMERA_WIDTH_DIV1  640
#define CAMERA_HEIGHT_DIV1 480

struct camera_config {
	uint32_t format;
	uint16_t width;
	uint16_t height;
};

struct camera {
	OV7670_host driver_host;
	struct camera_config config;
	struct camera_platform_config *platform;
	uint8_t *frame_buffer;
};

struct camera_platform_config {
	int (*i2c_write_blocking)(void *i2c_handle, uint8_t addr, const uint8_t *src, size_t len);
	int (*i2c_read_blocking)(void *i2c_handle, uint8_t addr, uint8_t *src, size_t len);
	void *i2c_handle;
	uint xclk_pin;
	uint xclk_divider;
	uint vsync_pin;
	uint href_pin;
	uint pclk_pin;
	uint data_pins[8];
};

uint8_t camera_pixels_per_chunk(uint32_t format);
int camera_do_frame(struct camera *camera, uint8_t *buf, uint16_t width, uint16_t height);
int camera_init(struct camera *camera, struct camera_platform_config *params, OV7670_size size);
int camera_configure(struct camera *camera, uint32_t format, uint16_t width, uint16_t height, OV7670_size size);
int camera_capture_blocking(struct camera *camera, uint8_t *into, uint16_t width, uint16_t height);
void camera_dma_start(uint8_t *framebuffer, size_t frame_size);
void camera_capture_frame(uint8_t *buf, size_t frame_size);

#endif /* __CAMERA_H__ */
