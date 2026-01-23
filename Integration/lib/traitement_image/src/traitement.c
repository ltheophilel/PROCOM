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

int choix_direction_binaire(uint8_t *bw_image, int width, int height)
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

    // printf("Équation de la droite : y = %.3fx + %.3f\n", m, p);
    double direction = GAIN_REGLAGE*m;
    // printf("Commande = %.3f\n", direction);
    // return 0;

    // DECISION BINAIRE
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
    return (right_count < left_count) ? -1 : 1; 
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
                int x = height - row;
                int y = col - width/2;
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

    double denom = (n * sum_x2 - sum_x * sum_x);
    if (fabs(denom) < 1e-9)
    {
        fprintf(stderr, "Denominateur nul dans calcul de pente.\n");
        return 0.0;
    }

    double m = (n * sum_xy - sum_x * sum_y) / denom;
    double p = (sum_y - m * sum_x) / (double)n;

    // printf("Équation de la droite : y = %.6fx + %.6f\n", m, p);


    double angle = atan((m*PROFONDEUR+p)/PROFONDEUR) * (180.0 / PI);
    // double angle = atan(m) * (180.0 / PI);
    // printf("Angle brut = %.3f degrés\n", angle);

    double angle_avg = add_to_moving_average(angle, angle_buffer, &angle_index, MOVING_AVG_SIZE);
    // printf("Angle lissé = %.3f degrés\n", angle_avg);

    return angle_avg;
}

int ligne_detectee(uint8_t *bw_image, int width, int height)
{
    // ? Axe x vers le haut, axe y vers la droite (robot en bas au milieu)
    long n = 0;

    // Trouve les pixels noirs (valeur 0)
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            if (bw_image[row*width+col] == 0)
            {
                n++;
            }
        }
    }
    if (n < SEUIL_DETECTION_LIGNE)
    {
        return 0; // ligne non détectée
    }
    else
    {
        return 1; // ligne détectée
    }
}

void chercher_ligne()
{
    int passage = 0;
    int ligne_trouvee = 0;

    // Reculer un peu pour revenir près de la ligne
    motor_set_direction(&moteur0, 0);
    motor_set_direction(&moteur1, 1);

    motor_set_pwm_brut(&moteur0, pwm_lookup_for_rpm(V_ROTATION*5));
    motor_set_pwm_brut(&moteur1, pwm_lookup_for_rpm(V_ROTATION*5));

    sleep_ms(500); // Reculer pendant 0.5 seconde

    motor_set_pwm_brut(&moteur0, 0);
    motor_set_pwm_brut(&moteur1, 0);

    motor_set_direction(&moteur0, 1);
    motor_set_direction(&moteur1, 0);

    while (1)
    {
        // 1. Vérifier si la ligne est détectée
        if (ligne_detectee(image_bw, MAX_WIDTH, MAX_HEIGHT))
        {
            return; // ligne trouvée, sortir de la fonction
        }

        // 2. Tourner légèrement à droite
        motor_set_pwm_brut(&moteur0, 0);
        motor_set_pwm_brut(&moteur1, pwm_lookup_for_rpm(V_ROTATION));

        passage++;
        sleep_ms(100); // Attendre un peu pour que le robot tourne

        // 3. Si on a fait un tour complet (360°), reculer un peu et recommencer
        if (passage >= 100)
        {
            // Reculer de 5 cm
            passage = 0.0;
        }
    }
}