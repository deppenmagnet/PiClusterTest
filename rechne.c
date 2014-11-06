#include "rechne.h"

void *rechne(void *args){
    args_t *param = (args_t*)args;  /* die dem Thread übergebenen Argumente */
    long double thread_local_h = 0.0, 
                thread_local_ze= 0.0, 
                thread_local_x = 0.0, 
                thread_local_integral = 0.0;
    void (*f)(const long double*, long double*); /* die Funktion */
    int i;
    /* Würde auch mit weniger Variablen gehen, aber dann wird das Programm
     * aufgrund der Unterteilung in MPI-Prozesse UND Threads undebuggable */
    thread_local_integral = 0;
    f = param->funktion;
    thread_local_h = (param->thread_local_b - param->thread_local_a)/(long double)param->thread_local_n;
    /* "Startwert" = Mittelwert von Unter- und Obergrenze */
    (*f)(&(param->thread_local_a), &thread_local_ze);
    thread_local_integral += thread_local_ze;
    (*f)(&(param->thread_local_b), &thread_local_ze);
    thread_local_integral += thread_local_ze;
    thread_local_integral /= 2.0;

    /* jetzt von links beginnen und alle Trapezoide berechnen */
    thread_local_x = param->thread_local_a;
    for(i = 1; i < param->thread_local_n; i++){
        thread_local_x += thread_local_h;
        (*f)(&thread_local_x, &thread_local_ze);
        thread_local_integral += thread_local_ze;
    }
    thread_local_integral *= thread_local_h;
    param->thread_local_integral = thread_local_integral;
    pthread_exit(NULL); /* Thread-Ende */
}

