#ifndef RTPS_MODE_H
#define RTPS_MODE_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "quadrature_encoder.pio.h"
#include "pwm_pico.h"
#include "eqep_pico.h"


#define CTRL_LOOP_PERIOD 1000

#define PIN_MOT0_EQEP_AB  2
#define PIN_MOT1_EQEP_AB  6
#define PIN_MOT2_EQEP_AB 10
#define PIN_MOT3_EQEP_AB 16

#define EQEP_MOT0_OFFSET 0
#define EQEP_MOT1_OFFSET 0
#define EQEP_MOT2_OFFSET 0
#define EQEP_MOT3_OFFSET 0

#define INC_TO_MM_LENGHT 0.00326552
#define INC_TO_M_LENGHT 0.00000326552

struct status
{
    float mot_pos[4];
    float mot_des_pos[4];
    float mot_vit[4];
    float mot_des_vit[4];
    struct eqep_pico eqep[4];
};

void init(struct rtps_status * rtps);

void control_loop(struct rtps_status * rtps);

#endif