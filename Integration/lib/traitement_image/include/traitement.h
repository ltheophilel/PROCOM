#ifndef __TRAITEMENT_H__
#define __TRAITEMENT_H__

#include <stdint.h>
#include <math.h>

#define MAX_WIDTH 80
#define MAX_HEIGHT 60
#define PROFONDEUR 40  // Distance en pixel pour le calcul de l'angle
#define PI 3.14159265358979323846
#define MOVING_AVG_SIZE 2


#define GAIN_REGLAGE 1

int seuillage(uint8_t *image, uint8_t *bw_image,
              int width, int height);

int choix_direction_binaire(uint8_t *bw_image,
                    int width, int height);

double trouver_angle(uint8_t *bw_image,
                     int width, int height);


#endif