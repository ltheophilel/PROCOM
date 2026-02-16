

#define L 0.29 // Distance entre les deux roues, en m
#define R 0.033 // Rayon d'une roue, en m
#define PI 3.14159265358979323846
#define MAX_RPM 290.0f               // Vitesse maximale attendue (pour la table de lookup)


int* get_vitesse_mot(float Vmoy, float angle, float T) {
    static int vitesse[2];
    vitesse[0] = (Vmoy/R + (angle*L)/(2*R*T))*(60.0/(2*PI));  // en rpm
    vitesse[1] = (Vmoy/R - (angle*L)/(2*R*T))*(60.0/(2*PI));  // en rpm
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


