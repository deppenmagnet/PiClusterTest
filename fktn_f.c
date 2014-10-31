/*****************************************************
 * fktn_f: Die Funktion f(x) = x^2
 *****************************************************/

#include <math.h>
#include "fktn_f.h"
#include <stddef.h>

void fktn_f(const long double *x, long double *y){
    *y = pow(*x, 2.0);
}
