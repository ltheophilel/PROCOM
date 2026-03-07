#ifndef __TRAITEMENT_H__
#define __TRAITEMENT_H__

#include <stdint.h>
#include <math.h>

#define MAX_WIDTH 80 // Largeur de l'image traitée pour la détection de ligne
#define MAX_HEIGHT 60 // Hauteur de l'image traitée pour la détection de ligne
// #define PROFONDEUR 20 //40  // Distance en pixel pour le calcul de l'angle
// #define SEUIL 128
#define PI 3.141592 // 65358979323846
#define SEUIL_DETECTION_LIGNE 5 // Nombre de pixels noirs minimum pour considérer qu'une ligne est détectée.

// Fonction de seuillage avec suppression du bruit : convertit l'image en niveaux de gris
// en image binaire et élimine les bandes noires horizontales trop fines (< 3 pixels)
int seuillage(uint8_t *image, uint8_t *bw_image,
              int width, int height, short SEUIL);

// Fonction de seuillage avec codage RLE (Run-Length Encoding) pour transmission optimisée
// Compte les séquences consécutives de pixels de même couleur et les encode
int seuillage_pour_transmission(uint8_t *image,
                                 uint8_t *coded_image,
                                 int width, int height, uint16_t *len_coded_image, short SEUIL);

// Fonction de conversion octet vers bits pour transmission : compresse 8 pixels en 1 octet
// Chaque bit représente un pixel (1=blanc, 0=noir) après seuillage
int byte_to_bit_for_transmission(uint8_t *image,
                                 uint8_t *bit_image,
                                 int width, int height, short SEUIL);

// Fonction de choix de direction binaire : analyse la répartition des pixels noirs
// pour décider si tourner à gauche (-1) ou à droite (1) pour suivre la ligne (A enlever ?)
int choix_direction_binaire(uint8_t *bw_image,
                    int width, int height);

// Fonction pour trouver l'angle de la ligne détectée en utilisant la régression linéaire
// Retourne un tableau [angle, p, m] où angle est en degrés
double* trouver_angle(uint8_t *bw_image,
                     int width, int height, int PROFONDEUR);

// Fonction de détection de ligne : vérifie si le nombre de pixels noirs dépasse un seuil
// Renvoie 1 si ligne détectée, 0 sinon
int ligne_detectee(uint8_t *bw_image, int width, int height);

// Fonction pour appliquer une transformation homographique à un point 2D
// Transforme les coordonnées (x, y) en (x_aplati, y_aplati) via la matrice d'homographie
void appliquerHomographie(double homographie[3][3], double x, double y, double *x_aplati, double *y_aplati);

// Fonction d'aplatissement : applique une homographie pour corriger la perspective
// Transforme la ligne détectée et recalcule ses paramètres après aplatissement
double* aplatir(double angle, double p, double m, int PROFONDEUR);

#endif