#ifndef __TRAITEMENT_H__
#define __TRAITEMENT_H__

#include <stdint.h>

#define MAX_WIDTH 640
#define MAX_HEIGHT 480

#define GAIN_REGLAGE 1

int seuillage(uint8_t *image, uint8_t *bw_image,
              int width, int height);

int choix_direction(uint8_t *bw_image,
                    int width, int height);

#endif