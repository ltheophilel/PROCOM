#include "../include/traitement.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Fonction de seuillage avec suppression du bruit : convertit l'image en niveaux de gris en image binaire
// et élimine les bandes noires horizontales trop fines (< 3 pixels)
int seuillage(uint8_t *image, uint8_t *bw_image, int width, int height, short SEUIL) {
    // 1. Seuillage classique : pixels > SEUIL deviennent blancs (255), sinon noirs (0)
    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            bw_image[x * width + y] = (image[x * width + y] > SEUIL) ? 255 : 0;
        }
    }

    // 2. Suppression des bandes noires horizontales trop fines (épaisseur < 3)
    // Parcourt chaque colonne pour détecter et supprimer les bandes de bruit
    for (int y = 0; y < width; y++) {  // Pour chaque colonne
        int x = 0;
        while (x < height) {
            if (bw_image[x * width + y] == 0) {  // Si pixel noir détecté
                int start = x;
                // Mesure la hauteur de la bande noire consécutive
                while (x < height && bw_image[x * width + y] == 0) {
                    x++;
                }
                int band_height = x - start;
                // Si la bande est trop fine (< 3 pixels), la supprimer en la mettant en blanc
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

// Fonction de seuillage avec codage RLE (Run-Length Encoding) pour transmission optimisée
// Compte les séquences consécutives de pixels de même couleur et les encode
int seuillage_pour_transmission(uint8_t *image,
              uint8_t *coded_image,
              int width, int height, uint16_t *len_coded_image, short SEUIL)
{
    uint16_t cpt_color = 0; // Compteur de pixels de la même couleur
    uint8_t cpt_alternance_line = 0; // Compteur d'alternances par ligne
    uint16_t index_coded_image = 0; // Index dans le buffer codé
    uint16_t max_data = 65535; // Valeur max pour 16 bits (2^16 - 1)
    uint16_t max_transmission_data = 1024; // Taille max du buffer de transmission
    unsigned char current_color = 0; // Couleur actuelle (0=noir, 1=blanc)
    unsigned char former_color = 1; // Couleur précédente, commence par blanc
    for (int x = 0; x < height; x++)
    {
        cpt_alternance_line = 0;
        for (int y = 0; y < width; y++)
        {
            // Déterminer la couleur binaire du pixel
            if (image[x*width+y] > SEUIL) current_color = 1;
            else current_color = 0;
            // Si même couleur que précédente, incrémenter le compteur
            if (current_color == former_color && cpt_color < max_data) {
                    cpt_color++;
                } else {
                    // Écrire le compteur en 2 octets (big-endian)
                    coded_image[index_coded_image++] = (cpt_color >> 8) & 0xFF; 
                    coded_image[index_coded_image++] = cpt_color & 0xFF;     
                    cpt_color = 1; // Réinitialiser pour la nouvelle couleur
                    cpt_alternance_line++;
                    former_color = current_color;
                }
            *len_coded_image = index_coded_image;
            // Vérifier si le buffer n'est pas trop plein
            if (*len_coded_image >= max_transmission_data) {
                printf("Erreur : image codée trop grande pour le buffer.\n");
                return -1;
            }
        }
    }
    // Écrire le dernier compteur
    coded_image[index_coded_image++] = (cpt_color >> 8) & 0xFF; 
    coded_image[index_coded_image++] = cpt_color & 0xFF;
    *len_coded_image = index_coded_image;
    return 0;
}

// Fonction de conversion octet vers bits pour transmission : compresse 8 pixels en 1 octet
// Chaque bit représente un pixel (1=blanc, 0=noir) après seuillage
int byte_to_bit_for_transmission(uint8_t *image,
                              uint8_t *bit_image,
                              int width, int height, short SEUIL)
{
    int index_bit_image = 0;
    // Traiter l'image par groupes de 8 pixels
    for (int i = 0; i < width * height; i+=8)
    {
        // Construire un octet où chaque bit correspond à un pixel seuillé
        bit_image[index_bit_image++] = 
                                    ((image[i]   > SEUIL) << 7)  // Bit 7 (MSB)
                                    | ((image[i+1] > SEUIL) << 6) // Bit 6
                                    | ((image[i+2] > SEUIL) << 5) // Bit 5
                                    | ((image[i+3] > SEUIL) << 4) // Bit 4
                                    | ((image[i+4] > SEUIL) << 3) // Bit 3
                                    | ((image[i+5] > SEUIL) << 2) // Bit 2
                                    | ((image[i+6] > SEUIL) << 1) // Bit 1
                                    | ((image[i+7] > SEUIL) << 0); // Bit 0 (LSB)
    }
    return 0;
}

// Fonction de choix de direction binaire : analyse la répartition des pixels noirs
// pour décider si tourner à gauche (-1) ou à droite (1) pour suivre la ligne
int choix_direction_binaire(uint8_t *bw_image, int width, int height)
{
    // Calcul de régression linéaire sur les pixels noirs pour estimer la pente
    // Axe x vers le haut (avancée), axe y vers la droite (décalage latéral)
    long sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    long n = 0;

    // Collecter les coordonnées des pixels noirs
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            if (bw_image[row*width+col] == 0)
            {
                int x = row;    // Distance vers l'avant
                int y = col;    // Décalage latéral
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
        return -1; // Erreur
    }

    // Calcul de la régression linéaire : y = m*x + p
    double m = -(n * (double)sum_xy - (double)sum_x * (double)sum_y) /
               (n * (double)sum_x2 - (double)sum_x * (double)sum_x);
    double p = ((double)sum_y - m * (double)sum_x) / n;
    // printf("Équation de la droite : y = %.3fx + %.3f\n", m, p);

    // DÉCISION BINAIRE : compter les pixels noirs à gauche et à droite du centre
    int right_count = 0, left_count = 0;

    for (int x = 0; x < height; x++) {
        for (int y = 0; y < width; y++) {
            if (bw_image[x*width+y] == 0) 
            {
                if(y > width/2) right_count++; // À droite du centre
                else left_count++; // À gauche du centre
            }
        }
    }

    printf("Pixels noirs à droite : %d, à gauche : %d\n", right_count, left_count);
    // Retourner -1 si plus de pixels à gauche (tourner à gauche), 1 sinon
    return (right_count < left_count) ? -1 : 1; 
}

// Fonction pour trouver l'angle de la ligne détectée en utilisant la régression linéaire
// Retourne un tableau [angle, p, m] où angle est en degrés
double* trouver_angle(uint8_t *bw_image, int width, int height, int PROFONDEUR)
{
    // Allocation du tableau résultat [angle, ordonnée à l'origine, pente]
    double * apm = malloc(3 * sizeof(double));
    apm[0] = 0.0;
    apm[1] = 0.0;
    apm[2] = 0.0;
    long sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    long n = 0;

    // Collecter les coordonnées des pixels noirs
    // Système de coordonnées : x = distance vers l'avant (height - row), y = décalage latéral (col - width/2)
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            if (bw_image[row*width+col] == 0)
            {
                int x = height - row; // Distance vers l'avant (robot en bas)
                int y = col - width/2; // Décalage latéral par rapport au centre
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
        return apm; // Retourner valeurs par défaut
    }

    // Calcul du dénominateur pour la régression linéaire
    double denom = (n * sum_x2 - sum_x * sum_x);
    if (fabs(denom) < 1e-9)
    {
        fprintf(stderr, "Denominateur nul dans calcul de pente.\n");
        return apm;
    }

    // Calcul de la pente m et de l'ordonnée à l'origine p : y = m*x + p
    double m = (n * sum_xy - sum_x * sum_y) / denom;
    double p = (sum_y - m * sum_x) / (double)n;

    // printf("Équation de la droite : y = %.6fx + %.6f\n", m, p);

    // Calcul de l'angle en degrés : atan de la pente ajustée par PROFONDEUR
    double angle = atan((m*PROFONDEUR+p)/PROFONDEUR) * (180.0 / PI);
    // double angle = atan(m) * (180.0 / PI);
    // printf("Angle = %.3f degrés\n", angle);
    apm[0] = angle;
    apm[1] = p;
    apm[2] = m;
    return apm;
}

// Fonction de détection de ligne : vérifie si le nombre de pixels noirs dépasse un seuil
int ligne_detectee(uint8_t *bw_image, int width, int height)
{
    // Compter le nombre total de pixels noirs dans l'image
    long n = 0;

    // Parcourir tous les pixels
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
    // Si le nombre de pixels noirs est inférieur au seuil, ligne non détectée
    if (n < SEUIL_DETECTION_LIGNE)
    {
        return 0; // ligne non détectée
    }
    else
    {
        return 1; // ligne détectée
    }
}

// Fonction pour appliquer une transformation homographique à un point 2D
// Transforme les coordonnées (x, y) en (x_aplati, y_aplati) via la matrice d'homographie
void appliquerHomographie(double homographie[3][3], double x, double y, double *x_aplati, double *y_aplati) {
    // Calcul des coordonnées homogènes après transformation
    double xh = homographie[0][0] * x + homographie[0][1] * y + homographie[0][2];
    double yh = homographie[1][0] * x + homographie[1][1] * y + homographie[1][2];
    double wh = homographie[2][0] * x + homographie[2][1] * y + homographie[2][2];

    // Normalisation
    *x_aplati = xh / wh;
    *y_aplati = yh / wh;
}

// Fonction d'aplatissement : applique une homographie pour corriger la perspective
// Transforme la ligne détectée et recalcule ses paramètres après aplatissement
double* aplatir(double angle, double p, double m, int PROFONDEUR) {
    // Allocation du tableau résultat [angle_aplati, p_aplati, m_aplati]
    double *apm = malloc(3 * sizeof(double));
    if (apm == NULL) {
        return NULL; // Échec de l'allocation mémoire
    }

    // Matrice d'homographie prédéfinie pour la correction de perspective
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

    // Appliquer l'homographie aux deux points de la droite
    double x1_aplati, y1_aplati, x2_aplati, y2_aplati;
    appliquerHomographie(homographie, x1, y1, &x1_aplati, &y1_aplati);
    appliquerHomographie(homographie, x2, y2, &x2_aplati, &y2_aplati);

    // Vérifier la division par zéro (droite verticale après transformation)
    if (fabs(x2_aplati - x1_aplati) < 1e-10) {
        free(apm);
        return NULL; // Droite verticale après transformation
    }

    // Recalculer la pente et l'ordonnée à l'origine après transformation
    double m_aplati = (y2_aplati - y1_aplati) / (x2_aplati - x1_aplati);
    double p_aplati = y1_aplati - m_aplati * x1_aplati;

    // Calcul de l'angle après aplatissement (en degrés)
    double angle_aplati = atan(y2_aplati / x2_aplati) * (180.0 / PI);

    apm[0] = angle_aplati;
    apm[1] = p_aplati;
    apm[2] = m_aplati;
    return apm;
}