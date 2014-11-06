#include "print_info.h"
#include <stdio.h>

void print_info(void){
    printf("*****************************************************\n");
    printf("* Integralberechnungen durch Pi-Cluster             \n");
    printf("*****************************************************\n");
    printf("Command-Line-Optionen:                            \n");
    printf("-t <int>\tFestlegen der zu verwendenden Threads   \n");
    printf("-a <float>\tFestlegen der Integral-Untergrenze\n");
    printf("-b <float>\tFestlegen der Integral-Obergrenze\n");
    printf("-n <int>\tFestlegen der Trapezoid-Anzahl\n");
    printf("-f <int>\tFestlegen der zu integrierenden Funktion\n");
    printf("\t0 : f(x)= x^2\n");
    printf("\t1 : f(x)= 4/(1+x^2\n");
    printf("\t2 : f(x)= x/(1+e^x)\n");
    printf("\nDefaultwerte:\n");
    printf("a = 0.0, b = 1.0, n = 10000000, f = 0\n");
}
