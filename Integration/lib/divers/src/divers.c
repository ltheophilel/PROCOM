#include "../include/divers.h"

char* trim_whitespace_divers(char *line) {
    // Supprime les espaces en début et fin de chaîne
    char *s = line;
    while (*s && (*s == ' ' || *s == '\t')) s++;
    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\0' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    return s;
}

int estNombreEntier(const char *s) {
    // Vérifie si la chaîne représente un nombre entier valide
    char *end;
    long val = strtol(s, &end, 10);
    // Vérifie si la conversion a consommé toute la chaîne et qu'il n'y a pas eu d'erreur
    return (*end == '\0' && s != end);
}


int signe(double x) {
    // Fonction pour obtenir le signe d'un nombre : -1 pour négatif, 0 pour zéro, 1 pour positif
    if (fabs(x) < DBL_EPSILON) {
        return 0; // Considéré comme nul
    } else if (x > 0.0) {
        return 1;
    } else {
        return -1;
    }
}