/*********************************************
 * fktn_g: f(x) = 4/(1+x^2)
 *********************************************/

#include <math.h>
#include "fktn_g.h"
#include <stddef.h>

void fktn_g(const long double *x, long double *y){
    *y = 4.0 / (1.0 + pow(*x,2.0));
}
