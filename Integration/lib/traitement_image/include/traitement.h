#ifndef __TRAITEMENT_H__
#define __TRAITEMENT_H__

#include <stdint.h>
#include <math.h>

#define MAX_WIDTH 80
#define MAX_HEIGHT 60
#define PROFONDEUR 20 //40  // Distance en pixel pour le calcul de l'angle
// #define SEUIL 128
#define PI 3.14159265358979323846
#define MOVING_AVG_SIZE 1
#define SEUIL_DETECTION_LIGNE 5

#define GAIN_REGLAGE 1

int seuillage(uint8_t *image, uint8_t *bw_image,
              int width, int height, short SEUIL);

int seuillage_pour_transmission(uint8_t *image,
                                 uint8_t *coded_image,
                                 int width, int height, uint16_t *len_coded_image, short SEUIL);

int byte_to_bit_for_transmission(uint8_t *image,
                                 uint8_t *bit_image,
                                 int width, int height, short SEUIL);

int choix_direction_binaire(uint8_t *bw_image,
                    int width, int height);

double* trouver_angle(uint8_t *bw_image,
                     int width, int height);

int ligne_detectee(uint8_t *bw_image, int width, int height);

double* aplatir(double angle, double p, double m);

#endif