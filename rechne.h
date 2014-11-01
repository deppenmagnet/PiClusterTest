#ifndef __rechne_h__
#define __rechne_h__

#include <pthread.h>
#include <stdio.h>

typedef struct args_typ{
    void (*funktion)(const long double*, long double*);
    long double thread_local_a;
    long double thread_local_b;
    long double thread_local_n;
    long double thread_local_integral;
}args_t;


extern void *rechne(void *arg);

#endif /* __rechne_h__ */
