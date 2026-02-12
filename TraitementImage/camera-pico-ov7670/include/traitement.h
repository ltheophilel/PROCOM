#ifndef __TRAITEMENT_H__
#define __TRAITEMENT_H__

#include <stdint.h>
#include <math.h>

#define MAX_WIDTH 640
#define MAX_HEIGHT 480

#define GAIN_REGLAGE 1
#define PI 3.14159265358979323846
#define MOVING_AVG_SIZE 20
#define PROFONDEUR 400  // Distance en pixel pour le calcul de l'angle

int seuillage(uint8_t *image, uint8_t *bw_image,
              int width, int height);

int choix_direction(uint8_t *bw_image,
                    int width, int height);

double add_to_moving_average(double value, double *buffer, int *index, int size);

// Nouvelle API : séparation calcul d'angle et choix de direction
double trouver_angle(uint8_t *bw_image,
                     int width, int height);

int donner_direction(double angle);

#endif /*__TRAITEMENT_H__*/