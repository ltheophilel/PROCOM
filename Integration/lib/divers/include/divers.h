#ifndef __DIVERS_H__
#define __DIVERS_H__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define DBL_EPSILON 0.00000000000001

char* trim_whitespace_divers(char *line);
int estNombreEntier(const char *s);
int signe(double x);
#endif