#include <math.h>

#define L 0.29 // Distance entre les deux roues, en m
#define R 0.033 // Rayon d'une roue, en m
#define H 0.30 // distance entre la caméra et l'axe de rotation du robot, en m
#define PI 3.14159265358979323846
#define MAX_RPM 290.0f               // Vitesse maximale attendue (pour la table de lookup)

#define CAMERA_SUR_AXE_ROUES true // true : la caméra est sur l'axe des roues, false : la caméra est en avant de l'axe des roues (distance H)


int* get_vitesse_mot(float Vmoy, float angle, float T) {
    static int vitesse[2];
    if (CAMERA_SUR_AXE_ROUES) {
        vitesse[0] = (Vmoy/R + (angle*L)/(2*R*T))*(60.0/(2*PI));  // en rpm
        vitesse[1] = (Vmoy/R - (angle*L)/(2*R*T))*(60.0/(2*PI));  // en rpm
    }
    else {
        vitesse[0] = -angle*L/(2*T*R) + sqrt(pow(angle*H/T, 2) + pow(Vmoy, 2))/R; // en rad/s
        vitesse[1] = angle*L/(2*T*R) + sqrt(pow(angle*H/T, 2) + pow(Vmoy, 2))/R;  // en rad/s
        vitesse[0] = -vitesse[0]*(60.0/(2*PI)); // conversion en rpm + inversion
        vitesse[1] = -vitesse[1]*(60.0/(2*PI)); // conversion en rpm + inversion
    }

    if (vitesse[0] > MAX_RPM) {
        vitesse[0] = MAX_RPM;
        vitesse[1] = vitesse[1] - (vitesse[0] - MAX_RPM);
    }
    if (vitesse[1] > MAX_RPM) {
        vitesse[1] = MAX_RPM;
        vitesse[0] = vitesse[0] - (vitesse[1] - MAX_RPM);
    }
    return vitesse;
}


