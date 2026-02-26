#include "mode.h"



void init(struct status * rtps)
{
    int32_t current[4] = {0,0,0,0};

    // eqep:
    eqep_pico_init();
    eqep_pico_add(&rtps->eqep[0], 0, PIN_MOT0_EQEP_AB);
    eqep_pico_add(&rtps->eqep[1], 0, PIN_MOT1_EQEP_AB);
    eqep_pico_add(&rtps->eqep[2], 0, PIN_MOT2_EQEP_AB);
    eqep_pico_add(&rtps->eqep[3], 0, PIN_MOT3_EQEP_AB);

    sleep_ms(2000);
}

void control_loop(struct status * rtps)
{

    uint64_t ref_time_ctrl;
    int32_t dt_ctrl;

    int32_t mot_pos[3]={0,0,0};
    int32_t mot_posmdt[3]={0,0,0};

    float mot_vit[3]={0.0,0.0,0.0};
    float mot_vitmdt[3]={0.0,0.0,0.0};
    float mot_vitm2dt[3]={0.0,0.0,0.0};
    float mot_vitm3dt[3]={0.0,0.0,0.0};

    sleep_ms(500);
    ref_time_ctrl = time_us_64()+ CTRL_LOOP_PERIOD;

    while(1)
    {
        // Positions:
        mot_pos[0] = MOT0_EQEP_DIR*eqep_pico_get_count(&rtps->eqep[0]);
        mot_pos[1] = MOT1_EQEP_DIR*eqep_pico_get_count(&rtps->eqep[1]);
        mot_pos[2] = MOT2_EQEP_DIR*eqep_pico_get_count(&rtps->eqep[2]);

        // Speeds:
        mot_vit[0] = (MOTOR_SPEED_COEFF1_O4*INC_TO_MM_LENGHT*((float)(mot_pos[0]-mot_posmdt[0])))
                   + (MOTOR_SPEED_COEFF2_O4*mot_vitmdt[0])
                   + (MOTOR_SPEED_COEFF3_O4*mot_vitm2dt[0])
                   + (MOTOR_SPEED_COEFF4_O4*mot_vitm3dt[0]);
        mot_vit[1] = (MOTOR_SPEED_COEFF1_O4*INC_TO_MM_LENGHT*((float)(mot_pos[1]-mot_posmdt[1])))
                   + (MOTOR_SPEED_COEFF2_O4*mot_vitmdt[1])
                   + (MOTOR_SPEED_COEFF3_O4*mot_vitm2dt[1])
                   + (MOTOR_SPEED_COEFF4_O4*mot_vitm3dt[1]);
        mot_vit[2] = (MOTOR_SPEED_COEFF1_O4*INC_TO_MM_LENGHT*((float)(mot_pos[2]-mot_posmdt[2])))
                   + (MOTOR_SPEED_COEFF2_O4*mot_vitmdt[2])
                   + (MOTOR_SPEED_COEFF3_O4*mot_vitm2dt[2])
                   + (MOTOR_SPEED_COEFF4_O4*mot_vitm3dt[2]);   

        
        // Update:
        mot_posmdt[0] = mot_pos[0];
        mot_vitm3dt[0] = mot_vitm2dt[0];
        mot_vitm2dt[0] = mot_vitmdt[0];
        mot_vitmdt[0] = mot_vit[0];

        mot_posmdt[1] = mot_pos[1];
        mot_vitm3dt[1] = mot_vitm2dt[1];
        mot_vitm2dt[1] = mot_vitmdt[1];
        mot_vitmdt[1] = mot_vit[1];

        mot_posmdt[2] = mot_pos[2];
        mot_vitm3dt[2] = mot_vitm2dt[2];
        mot_vitm2dt[2] = mot_vitmdt[2];
        mot_vitmdt[2] = mot_vit[2];

        // Waiting:
        while ((dt_ctrl=ref_time_ctrl-time_us_64()) > 0);
        ref_time_ctrl += LOOP_PERIOD;
    }               
}
