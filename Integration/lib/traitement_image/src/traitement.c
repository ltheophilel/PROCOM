#include "../include/traitement.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

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

int seuillage(uint8_t *image, uint8_t *bw_image, int width, int height, short SEUIL) {
    // 1. Seuillage classique
    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            bw_image[x * width + y] = (image[x * width + y] > SEUIL) ? 255 : 0;
        }
    }

    // 2. Suppression des bandes noires horizontales trop fines (épaisseur < 3)
    for (int y = 0; y < width; y++) {  // Pour chaque colonne
        int x = 0;
        while (x < height) {
            if (bw_image[x * width + y] == 0) {  // Si pixel noir
                int start = x;
                // Mesure la hauteur de la bande noire (combien de lignes consécutives)
                while (x < height && bw_image[x * width + y] == 0) {
                    x++;
                }
                int band_height = x - start;
                // Si l'épaisseur est inférieure à 3, on met en blanc
                if (band_height < 3) {
                    for (int i = start; i < x; i++) {
                        bw_image[i * width + y] = 255;
                    }
                }
            } else {
                x++;
            }
        }
    }
    return 0;
}

int seuillage_pour_transmission(uint8_t *image,
              uint8_t *coded_image,
              int width, int height, uint16_t *len_coded_image, short SEUIL)
{
    uint16_t cpt_color = 0;
    uint8_t cpt_alternance_line = 0;
    uint16_t index_coded_image = 0;
    uint16_t max_data = 65535; // 2^16 -1;
    uint16_t max_transmission_data = 1024;
    unsigned char current_color = 0;
    unsigned char former_color = 1; // commencer par du blanc
    for (int x = 0; x < height; x++)
    {
        cpt_alternance_line = 0;
        for (int y = 0; y < width; y++)
        {
            if (image[x*width+y] > SEUIL) current_color = 1;
            else current_color = 0;
            if (current_color == former_color && cpt_color < max_data) {
                    cpt_color++;
                } else {
                    coded_image[index_coded_image++] = (cpt_color >> 8) & 0xFF; 
                    coded_image[index_coded_image++] = cpt_color & 0xFF;     
                    cpt_color = 1;
                    cpt_alternance_line++;
                    former_color = current_color;
                }
            *len_coded_image = index_coded_image;
            if (*len_coded_image >= max_transmission_data) {
                printf("Erreur : image codée trop grande pour le buffer.\n");
                return -1;
            }
        }
    }
    coded_image[index_coded_image++] = (cpt_color >> 8) & 0xFF; 
    coded_image[index_coded_image++] = cpt_color & 0xFF;
    *len_coded_image = index_coded_image;
    return 0;
}

int byte_to_bit_for_transmission(uint8_t *image,
                              uint8_t *bit_image,
                              int width, int height, short SEUIL)
{
    int index_bit_image = 0;
    for (int i = 0; i < width * height; i+=8)
    {
        bit_image[index_bit_image++] = 
                                    ((image[i]   > SEUIL) << 7)
                                    | ((image[i+1] > SEUIL) << 6)
                                    | ((image[i+2] > SEUIL) << 5)
                                    | ((image[i+3] > SEUIL) << 4)
                                    | ((image[i+4] > SEUIL) << 3)
                                    | ((image[i+5] > SEUIL) << 2)
                                    | ((image[i+6] > SEUIL) << 1)
                                    | ((image[i+7] > SEUIL) << 0);
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


double* trouver_angle(uint8_t *bw_image, int width, int height, int PROFONDEUR)
{
    double * apm = malloc(3 * sizeof(double));
    apm[0] = 0.0;
    apm[1] = 0.0;
    apm[2] = INFINITY;
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
        return apm; // no reliable angle
    }

    double denom = (n * sum_x2 - sum_x * sum_x);
    if (fabs(denom) < 1e-9)
    {
        fprintf(stderr, "Denominateur nul dans calcul de pente.\n");
        return apm;
    }

    double m = (n * sum_xy - sum_x * sum_y) / denom;
    double p = (sum_y - m * sum_x) / (double)n;

    // printf("Équation de la droite : y = %.6fx + %.6f\n", m, p);


    double angle = atan((m*PROFONDEUR+p)/PROFONDEUR) * (180.0 / PI);
    // double angle = atan(m) * (180.0 / PI);
    // printf("Angle brut = %.3f degrés\n", angle);

    double angle_avg = add_to_moving_average(angle, angle_buffer, &angle_index, MOVING_AVG_SIZE);
    // printf("Angle lissé = %.3f degrés\n", angle_avg);
    apm[0] = angle_avg;
    apm[1] = p;
    apm[2] = m;
    return apm;
// }

//     return angle_avg;
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

// Fonction pour appliquer une homographie à un point 2D
void appliquerHomographie(double homographie[3][3], double x, double y, double *x_aplati, double *y_aplati) {
    // Coordonnées homogènes
    double xh = homographie[0][0] * x + homographie[0][1] * y + homographie[0][2];
    double yh = homographie[1][0] * x + homographie[1][1] * y + homographie[1][2];
    double wh = homographie[2][0] * x + homographie[2][1] * y + homographie[2][2];

    // Normalisation
    *x_aplati = xh / wh;
    *y_aplati = yh / wh;
}

double* aplatir(double angle, double p, double m, int PROFONDEUR) {
    double *apm = malloc(3 * sizeof(double));
    if (apm == NULL) {
        return NULL; // Échec de l'allocation
    }

    // Ta matrice d'homographie
    double homographie[3][3] = {
        {-1.20634921, 0.0, 0.0},
        {0.0, -0.66666667, 0.0},
        {-0.02301587, -0.0, 1.0}
    };

    // Points initiaux sur la droite y = m*x + p
    double x1 = 0.0;
    double y1 = m * x1 + p;
    double x2 = PROFONDEUR;
    double y2 = m * x2 + p;

    // Appliquer l'homographie
    double x1_aplati, y1_aplati, x2_aplati, y2_aplati;
    appliquerHomographie(homographie, x1, y1, &x1_aplati, &y1_aplati);
    appliquerHomographie(homographie, x2, y2, &x2_aplati, &y2_aplati);

    // Vérifier la division par zéro
    if (fabs(x2_aplati - x1_aplati) < 1e-10) {
        free(apm);
        return NULL; // Droite verticale après transformation
    }

    // Calcul de la pente et de l'ordonnée à l'origine
    double m_aplati = (y2_aplati - y1_aplati) / (x2_aplati - x1_aplati);
    double p_aplati = y1_aplati - m_aplati * x1_aplati;

    // Calcul de l'angle (en degrés)
    double angle_aplati = atan(y2_aplati / x2_aplati) * (180.0 / PI);

    apm[0] = angle_aplati;
    apm[1] = p_aplati;
    apm[2] = m_aplati;
    return apm;
}