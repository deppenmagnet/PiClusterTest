/******************************************************
 * Berechnung von Integralen mittels der Trapezregel
 * oder alternativ der Simpsons-Regel
 *
 * Die beiden Funktionen werden der Einfachheit halber
 * fest einprogrammiert. Vorgesehen ist f(x)=x^2 und
 * f(x) = 4/(1+x^2), da die einfach analytisch zu lösen
 * sind und ich die Rechenergebnisse prüfen kann :)
 *
 * Commandline-Parameter sind:
 * -t <nummer>: Anzahl der zu verwendenden Threads je
 *              Prozessor
 * -a <nummer>: linke Grenze (default = 0)
 * -b <nummer>: rechte Grenze (default = 1)
 * -n <nummer>: Anzahl der zu berechnenden Trapezoide
 ******************************************************/

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "fktn_f.h" /* x^2 */
#include "fktn_g.h" /* 4/(1+x^2) */
#include "identify_myself.h"
#include "rechne.h"

int main(int argc, char **argv){
    int t = 1;      /* Anzahl der Threads */
    long double a = 0;      /* linke Grenze */
    long double b = 1;      /* rechte Grenze */
    long int n = 1000000;   /* Anzahl der Trapezoide */
    clock_t start = 0.0;  /* Startzeit */
    clock_t ende = 0.0;   /* Endezeit */
    long double dauer = 0.0;  /* Dauer des Programmlaufs */
    long double integral=0.0; /* Ergebnis */
    int i = 0;          /* Zaehler */
    int c = 0;          /* Rückgabewert von getopt */
    opterr = 0;     /* Error-Var */
    void *thread_rc;/* Rückgabewert der Threads */
    int thread_status; /* Status der pthread-Funktionen */
    pthread_t *threads;
    args_t *args;

    printf("Starte die Integralberechnungen....\n");
    printf("Bitte warten....\n");

    identify_myself(&i); /* i wird kurzfristig missbraucht */
    switch(i){      /* Wie viele Threads verträgt die Kiste */
        case AMD_OCTA_CORE: t = 8*6; /* 8 Kerne, 6-fache Taktfrequenz eines Pi */
                            break;
        case RASPBERRY_PI:  t = 1;
                            break;
        case BANANA_PI:     t = 2;
                            break;
        default:            t = 1;
                            break;
    }

    /* Commandline Parameter auswerten */
    while((c = getopt(argc, argv, "t:a:b:n:")) != -1)
        switch(c){
            case 't':   t = atoi(optarg);
                        break;
            case 'a':   a = atof(optarg);
                        break;
            case 'b':   b = atof(optarg);
                        break;
            case 'n':   n = atoi(optarg);
                        break;
            default :   abort();
        }
#ifdef DEBUG
    printf("t = %d, a = %Lf, b = %Lf, n = %ld\n", t,a,b,n);
#endif

    /* Speicher für Threads allokieren */
    threads = malloc(sizeof(pthread_t)*t);

    /* Speicher für Argumente allokieren */
    args = malloc(sizeof(args_t));

    /* Zeitmessung starten */
    start = clock();
    for (i = 0; i < t; i++){ /* Rechen-Threads erzeugen */
        args->funktion = fktn_f;
        args->a = a;
        args->b = b;
        args->n = n;
        args->integral = 0.0;
        /* Threads erzeugen */
        thread_status = pthread_create(&threads[i], NULL, rechne, args);
        if(thread_status != 0){
            fprintf(stderr, "Error: Thread konnte nicht erzeugt werden\n");
            exit(EXIT_FAILURE);
        }
    }

    /* jetzt Threads joinen und auf Ende warten */
    for (i = 0; i < t; i++){
        thread_status = pthread_join(threads[i], &thread_rc);
        if(thread_status != 0){
            fprintf(stderr, "Error: Thread konnte nicht gejoined werden\n");
            exit(EXIT_FAILURE);
        }
        args = (args_t*)thread_rc;  /* NULL Pointer wieder derefernzieren */
        integral += args->integral; /* Teilergebnisse zusammenfügen */
    }

    ende = clock();
    dauer = (((long double)ende -(long double)start)/((long double)CLOCKS_PER_SEC))*(long double)1000;
    printf("Dauer der Berechnung mit %ld Trapezoiden mit %d Threads: %.2Lf Millisekunden\n",
            n,t,dauer);
    printf("Das Integral von f(x)=x^2 von %Lf bis %Lf ist: %Le\n",
            a, b, integral);
    exit(EXIT_SUCCESS);
}
