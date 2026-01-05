#include "../include/traitement.h"
#include <stdlib.h>
#include <stdio.h>

// Buffer for moving average of angle
static double angle_buffer[MOVING_AVG_SIZE] = {0};
static int angle_index = 0;

double add_to_moving_average(double value, double *buffer, int *index, int size)
{
    buffer[*index] = value;
    *index = (*index + 1) % size;
    
    double sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum += buffer[i];
    }
    return sum / size;
}

int seuillage(uint8_t *image,
              uint8_t *bw_image,
              int width, int height)
{
    for (int x = 0; x < height; x++)
    {
        for (int y = 0; y < width; y++)
        {
            bw_image[x*width+y] = (image[x*width+y] > 128) ? 255 : 0;
        }
    }
    return 0;
}

double trouver_angle(uint8_t *bw_image, int width, int height)
{
    long sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    long n = 0;

    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            if (bw_image[row*width+col] == 0)
            {
                int x = row;
                int y = col;
                sum_x += x;
                sum_y += y;
                sum_xy += x * y;
                sum_x2 += x * x;
                n++;
            }
        }
    }

    if (n < 2)
    {
        fprintf(stderr, "Pas assez de points pour calculer une droite.\n");
        return 0.0; // no reliable angle
    }

    double m = -(n * (double)sum_xy - (double)sum_x * (double)sum_y) /
               (n * (double)sum_x2 - (double)sum_x * (double)sum_x);
    double p = ((double)sum_y - m * (double)sum_x) / n;

    printf("Équation de la droite : y = %.3fx + %.3f\n", m, p);
    double angle = atan(m) * (180.0 / PI);
    printf("Angle brut = %.3f degrés\n", angle);

    double angle_avg = add_to_moving_average(angle, angle_buffer, &angle_index, MOVING_AVG_SIZE);
    printf("Angle lissé = %.3f degrés\n", angle_avg);

    return angle_avg;
}

int donner_direction(double angle)
{
    double command = GAIN_REGLAGE * angle;
    printf("Commande = %.3f\n", command);
    if (command < -5.0)
        return -1;
    else if (command > 5.0)
        return 1;
    else
        return 0;
}