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

#define CAMERA_WIDTH_DIV8  80
#define CAMERA_HEIGHT_DIV8 60

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
int camera_do_frame(struct camera *camera, uint8_t *buf);
int camera_init(struct camera *camera, struct camera_platform_config *params);
int camera_configure(struct camera *camera, uint32_t format, uint16_t width, uint16_t height);
int camera_capture_blocking(struct camera *camera, uint8_t *into);

#endif /* __CAMERA_H__ */
