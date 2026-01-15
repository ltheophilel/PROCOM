#include "../include/divers.h"

// Supprime les espaces en début et fin de chaîne
char* trim_whitespace_divers(char *line) {
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
    char *end;
    long val = strtol(s, &end, 10);
    // Vérifie si la conversion a consommé toute la chaîne et qu'il n'y a pas eu d'erreur
    return (*end == '\0' && s != end);
}


int signe(double x) {
    if (fabs(x) < DBL_EPSILON) {
        return 0; // Considéré comme nul
    } else if (x > 0.0) {
        return 1;
    } else {
        return -1;
    }
}