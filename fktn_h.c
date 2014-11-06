/*****************************************************
 * fktn_h: Die Funktion f(x) = sin(x)*e^x 
 *****************************************************/

#include <math.h>
#include "fktn_h.h"
#include <stddef.h>

void fktn_h(const long double *x, long double *y){
    *y = (sin(*x)*exp(*x));
}
