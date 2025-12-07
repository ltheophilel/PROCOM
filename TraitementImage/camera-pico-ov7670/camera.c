/**
 * Copyright (c) 2021-2022 Brian Starkey <stark3y@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "camera.h"
#include "ov7670.h"


static bool camera_detect(struct camera_platform_config *platform)
{
	const uint8_t reg = OV7670_REG_PID;
	uint8_t val = 0;

	int tries = 5;
	while (tries--) {
		int ret = platform->i2c_write_blocking(platform->i2c_handle, OV7670_ADDR, &reg, 1);
		if (ret != 1) {
			sleep_ms(100);
			continue;
		}

		ret = platform->i2c_read_blocking(platform->i2c_handle, OV7670_ADDR, &val, 1);
		if (ret != 1) {
			sleep_ms(100);
			continue;
		}

		if (val == 0x76) {
			break;
		}

		sleep_ms(100);
	}

	return val == 0x76;
}

int camera_init(struct camera *camera, struct camera_platform_config *params, OV7670_size size)
{
	OV7670_status status;

	*camera = (struct camera){ 0 };
	camera->platform = params;

	clock_gpio_init(params->xclk_pin, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, params->xclk_divider);

	camera->driver_host = (OV7670_host){
		.pins = &(OV7670_pins){
			.enable = -1,
			.reset = -1,
		},
		.platform = params,
	};

	// Some settling time for the clock
	sleep_ms(300);

	// Try and check that the camera is present
	if (!camera_detect(params)) {
		return -1;
	}

	// Note: Frame rate is ignored
	status = OV7670_begin(&camera->driver_host, OV7670_COLOR_YUV, size, 0.0);
	if (status != OV7670_STATUS_OK) {
		return -1;
	}
	return 0;
}

int camera_configure(struct camera *camera, uint32_t format, uint16_t width, uint16_t height, OV7670_size size)
{
	struct camera_platform_config *platform = camera->driver_host.platform;

	OV7670_set_format(platform, format);
	OV7670_set_size(platform, size);

	camera->config.format = format;
	camera->config.width = width;
	camera->config.height = height;

	return 0;
}

uint8_t camera_pixels_per_chunk(uint32_t format)
{
	switch (format) {
	case OV7670_COLOR_RGB:
		return 2;
	case OV7670_COLOR_YUV:
		return 2;
	default:
		return 1;
	}
}

int camera_do_frame(struct camera *camera, uint8_t *buf, uint16_t width, uint16_t height)
{
    uint8_t href_val = 0;
    uint8_t href_prev = 0;
    uint8_t pclk = 0;
    uint8_t pclk_prev = 0;
    int row = 0;
    int col = 0;

    // Attendre VSYNC (début d’image)
    while (!gpio_get(camera->platform->vsync_pin)) {}
    while ( gpio_get(camera->platform->vsync_pin)) {}

    while (row < height)
    {
        href_val = gpio_get(camera->platform->href_pin);

        // Début d’une ligne
        if (href_val && !href_prev)
        {
            col = 0;
        }

        // Pendant HREF → données valides
        if (href_val)
        {
            pclk = gpio_get(camera->platform->pclk_pin);

            if (pclk && !pclk_prev)
            {
                // ---- READ HIGH BYTE ----
                uint8_t high =
                    (gpio_get(camera->platform->data_pins[7]) << 7) |
                    (gpio_get(camera->platform->data_pins[6]) << 6) |
                    (gpio_get(camera->platform->data_pins[5]) << 5) |
                    (gpio_get(camera->platform->data_pins[4]) << 4) |
                    (gpio_get(camera->platform->data_pins[3]) << 3) |
                    (gpio_get(camera->platform->data_pins[2]) << 2) |
                    (gpio_get(camera->platform->data_pins[1]) << 1) |
                    (gpio_get(camera->platform->data_pins[0]));

                // attendre cycle suivant
                while (gpio_get(camera->platform->pclk_pin)) {}
                while (!gpio_get(camera->platform->pclk_pin)) {}

                // ---- READ LOW BYTE ----
                uint8_t low =
                    (gpio_get(camera->platform->data_pins[7]) << 7) |
                    (gpio_get(camera->platform->data_pins[6]) << 6) |
                    (gpio_get(camera->platform->data_pins[5]) << 5) |
                    (gpio_get(camera->platform->data_pins[4]) << 4) |
                    (gpio_get(camera->platform->data_pins[3]) << 3) |
                    (gpio_get(camera->platform->data_pins[2]) << 2) |
                    (gpio_get(camera->platform->data_pins[1]) << 1) |
                    (gpio_get(camera->platform->data_pins[0]));

                int idx = (row * width + col) * 2;
                buf[idx]     = high;
                buf[idx + 1] = low;

                col++;
            }
        }

        // Fin de ligne détectée
        if (!href_val && href_prev)
        {
            row++;
        }

        href_prev = href_val;
        pclk_prev = pclk;
    }

    return 0;
}

int camera_capture_blocking(struct camera *camera, uint8_t *into, uint16_t width, uint16_t height)
{
	return camera_do_frame(camera, into, width, height);
}

int OV7670_read_register(void *platform, uint8_t reg)
{
	struct camera_platform_config *pcfg = (struct camera_platform_config *)platform;
	uint8_t value;

	pcfg->i2c_write_blocking(pcfg->i2c_handle, OV7670_ADDR, &reg, 1);
	pcfg->i2c_read_blocking(pcfg->i2c_handle, OV7670_ADDR, &value, 1);

	return value;
}

void OV7670_write_register(void *platform, uint8_t reg, uint8_t value)
{
	struct camera_platform_config *pcfg = (struct camera_platform_config *)platform;

	pcfg->i2c_write_blocking(pcfg->i2c_handle, OV7670_ADDR, (uint8_t[]){ reg, value }, 2);
}
