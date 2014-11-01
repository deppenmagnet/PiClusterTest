#include "rechne.h"

void *rechne(void *args){
    args_t *param = (args_t*)args;  /* die dem Thread übergebenen Argumente */
    long double thread_local_a, 
                thread_local_b, 
                thread_local_n, 
                thread_local_h, 
                thread_local_ze, 
                thread_local_x, 
                thread_local_integral;
    void (*f)(const long double*, long double*); /* die Funktion */
    int i;
    long double *rc;
    /* Würde auch mit weniger Variablen gehen, aber dann wird das Programm
     * aufgrund der Unterteilung in MPI-Prozesse UND Threads undebuggable */
    thread_local_a = param->thread_local_a;
    thread_local_b = param->thread_local_b;
    thread_local_n = param->thread_local_n;
    thread_local_integral = 0;
    f = param->funktion;
    thread_local_h = (thread_local_b - thread_local_a)/thread_local_n;
    /* "Startwert" = "Großtrapez" */
    (*f)(&thread_local_a, &thread_local_ze);
    thread_local_integral += thread_local_ze;
    (*f)(&thread_local_b, &thread_local_ze);
    thread_local_integral += thread_local_ze;
    thread_local_integral /= 2.0;

    /* jetzt von links beginnen und alle Trapezoide berechnen */
    thread_local_x = thread_local_a;
    for(i = 1; i < thread_local_n; i++){
        thread_local_x += thread_local_h;
        (*f)(&thread_local_x, &thread_local_ze);
        thread_local_integral += thread_local_ze;
#ifdef DEBUG
        printf("Thread %Ld, Schritt: %d: %Lf\n", (long long int)pthread_self(), i, thread_local_integral*thread_local_h);
#endif
    }
    thread_local_integral *= thread_local_h;
    return (rc = &thread_local_integral);
}
