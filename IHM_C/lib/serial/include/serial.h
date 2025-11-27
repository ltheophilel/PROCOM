#ifndef IHM_H
#define IHM_H

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"

#define LINE_BUF_SIZE 128

void handle_serial_input(char *line, size_t *idx, void (*command_callback)(const char *));
void update_motor_value(uint64_t *t_us_previous, uint32_t t_us_between, bool which_mot, float *v_mot);

#endif /* IHM_H */