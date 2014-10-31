#include "rechne.h"

void *rechne(void *args){
    args_t *param = (args_t*)args;  /* die dem Thread übergebenen Argumente */
    long double a, b, n, h, ze, x, integral;
    void (*f)(const long double*, long double*); /* die Funktion */
    int i;

    a = param->a;
    b = param->b;
    n = param->n;
    integral = 0;
    f = param->funktion;
    h = (b-a)/n;
    /* "Startwert" = "Großtrapez" */
    (*f)(&a, &ze);
    integral += ze;
    (*f)(&b, &ze);
    integral += ze;
    integral /= 2.0;

    /* jetzt von links beginnen und alle Trapezoide berechnen */
    x = a;
    for(i = 1; i < n; i++){
        x += h;
        (*f)(&x, &ze);
        integral += ze;
#ifdef DEBUG
        printf("Thread %Ld, Schritt: %d: %Lf\n", (long long int)pthread_self(), i, integral*h);
#endif
    }
    integral *= h;
    param->integral = integral;
    return param;
}
