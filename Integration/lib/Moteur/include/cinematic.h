#ifndef PWM_LOOKUP_TABLE_H
#define PWM_LOOKUP_TABLE_H

#define L 0.29 // Distance entre les deux roues, en m
#define R 0.033 // Rayon d'une roue, en m
#define PI 3.14159265358979323846

int* get_vitesse_mot(float Vmoy, float angle, float T) {
    static int vitesse[2];
    vitesse[0] = (Vmoy/R + (angle*L)/(2*R*T))*(60.0/(2*PI));  // en rpm
    vitesse[1] = (Vmoy/R - (angle*L)/(2*R*T))*(60.0/(2*PI));  // en rpm
    return vitesse;
}

#endif 

