#include "../include/traitement.h"
#include <stdlib.h>
#include <stdio.h>


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
    double direction = GAIN_REGLAGE*m;
    printf("Commande = %.3f\n", direction);
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