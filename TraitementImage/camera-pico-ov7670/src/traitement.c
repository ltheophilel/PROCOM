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

int choix_direction(uint8_t *bw_image, int width, int height)
{
    // ? Axe x vers le haut, axe y vers la droite (robot en bas au milieu)
    long sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    long n = 0;

    // Récupère les pixels noirs (valeur 0)
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            if (bw_image[row*width+col] == 0)
            {
                int x = row;    // distance vers l'avant (avancée)
                int y = col;    // décalage latéral
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
        return -1;
    }

    // Régression linéaire : y = m*x + p
    double m = -(n * (double)sum_xy - (double)sum_x * (double)sum_y) /
               (n * (double)sum_x2 - (double)sum_x * (double)sum_x);
    double p = ((double)sum_y - m * (double)sum_x) / n;

    printf("Équation de la droite : y = %.3fx + %.3f\n", m, p);
    double angle = atan(m) * (180.0 / PI);
    // Apply moving average and return the averaged angle
    double angle_avg = add_to_moving_average(angle, angle_buffer, &angle_index, MOVING_AVG_SIZE);
    // For backwards compatibility, use the new API: retourner direction via donner_direction
    double final_angle = angle_avg;
    (void)final_angle; // keep compiler happy if not used below

    // We will delegate to the new functions by default below (wrapper)
    // Reuse the computed smoothed angle by temporarily storing it in a static variable
    // but better: call trouver_angle which does same computation — to avoid duplication
    // For simplicity, return the direction computed from angle_avg
    double command = GAIN_REGLAGE * angle_avg;
    printf("Angle brut = %.3f degrés\n", angle);
    printf("Angle lissé = %.3f degrés\n", angle_avg);
    printf("Commande = %.3f\n", command);

    if (command < -5.0)
        return -1;
    else if (command > 5.0)
        return 1;
    else
        return 0;

    /* // DECISION BINAIRE
    int right_count = 0, left_count = 0;

    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            if (bw_image[x*width+y] == 0) 
            {
                if(y > width/2) right_count++;
                else left_count++;
            }
        }
    }

    printf("Pixels noirs à droite : %d, à gauche : %d\n", right_count, left_count);
    return (right_count < left_count) ? -1 : 1; */
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