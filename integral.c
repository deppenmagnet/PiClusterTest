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
#include "mpi.h"

int main(int argc, char **argv){
    int nodes;              /* Anzahl der Knoten */
    int node;               /* lokale Knotennummer */
    int local_t = 1;        /* Anzahl der Threads (lokal je Knoten) */
    int total_t = 1;        /* Gesamtzahl der verwendeten Threads */
    long double a = 0;      /* linke Grenze */
    long double node_local_a; /* linke Grenze, knotenlokal */
    long double thread_local_a; /* linke Grenze, threadlokal */
    long double b = 1;      /* rechte Grenze */
    long double node_local_b; /*rechte Grenze, knotenlokal */
    long double thread_local_b; /* rechte Grenze, threadlokal */
    long int n = 10000000;   /* Anzahl der Trapezoide */
    long int node_local_n;       /* Je Knoten lokal zu berechnende Trapezoide */
    long int thread_local_n;    /* Je Thread lokal zu berechnende Trapezoide */
    long double h;          /* Trapezoid-Basis */
    clock_t start = 0.0;    /* Startzeit */
    clock_t ende = 0.0;     /* Endezeit */
    long double dauer = 0.0;  /* Dauer des Programmlaufs */
    long double integral = 0.0;   /* Das Gesamtergebnis */
    long double node_local_integral=0.0; /* Ergebnis je Knoten */
    int i = 0;              /* Zaehler */
    int c = 0;              /* Rückgabewert von getopt */
    opterr = 0;             /* Error-Var */
    void *thread_rc;        /* Rückgabewert der Threads */
    int thread_status;      /* Status der pthread-Funktionen */
    pthread_t *threads;     /* Speichert die Thread-ID's */
    args_t **args_array;           /* Argumente, die an die Threads übergeben werden */


    /* MPI Initialisierung */
    MPI_Init(&argc, &argv); 
    /* Anzahl der beteiligten Knoten ermitteln */
    MPI_Comm_size(MPI_COMM_WORLD, &nodes);
    /* Eigenen Knoten ermitteln */
    MPI_Comm_rank(MPI_COMM_WORLD, &node);
    
    /* Der Ordnung halber: Nur Knoten 0 soll IO-Operationen machen */
    if(node == 0){
        printf("Starte die Integralberechnungen....\n");
        printf("Beginne mit Cluster-Setup....\n");
    }

    /* Ermitteln, wieviele Threads insgesamt genutzt werden können */
    identify_myself(&i); /* i wird kurzfristig missbraucht */
    switch(i){      /* Wie viele Threads verträgt die Kiste */
        case AMD_OCTA_CORE: local_t = 8*6; /* 8 Kerne, 6-fache Taktfrequenz eines Pi */
                            break;
        case RASPBERRY_PI:  local_t = 1;
                            break;
        case BANANA_PI:     local_t = 2;
                            break;
        default:            local_t = 1;
                            break;
    }
    /* Commandline Parameter auswerten */
    while((c = getopt(argc, argv, "t:a:b:n:")) != -1)
        switch(c){
            case 't':   local_t = atoi(optarg);
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
    if(node == 0)
        printf("t = %d, a = %Lf, b = %Lf, n = %ld\n", local_t,a,b,n);
#endif

    /* Von allen Knoten abfragen, wie viele Threads sie wirklich nutzen */
    MPI_Reduce(&local_t, /* Sende die lokale Threadanzahl */ 
               &total_t, /* für die Gesamtanzahl */ 
               1,        /* einzelner Integerwert */
               MPI_INT,  /* Datentyp: MPI_INT */
               MPI_SUM,  /* Aggregatfunktion: Summiere */
               0,        /* Empfänger: Knoten 0 */
               MPI_COMM_WORLD); /* Sender: Alle beteiligten Knoten */
#ifdef DEBUG
    printf("Knoten %d meldet %d nutzbare Threads.\n", node, local_t);
#endif
    if(node == 0)
        printf("Knoten %d meldet: %d im Cluster nutzbare Threads\n", node, total_t);

    /* Speicher für Threads allokieren */
    threads = malloc(sizeof(pthread_t)*local_t);

    /* Speicher für Thread-Argumente allokieren */
    args_array = malloc(sizeof(args_t*)*local_t);

    /* Zeitmessung starten */
    if(node == 0)
        start = clock();

    /* Damit jeder Knoten gleichviel zu tun hat: Trapezoidanzahl hoch rechnen, so 
     * dass n ohne Rest durch nodes teilbar ist */
    if(n % nodes != 0)
        n = ((n / nodes)+1) * nodes;

    /* Jeder Knoten erzeugt seinen lokalen Arbeitsbereich innerhalb von a und b */
    h = (b-a)/n;            /* Basis der Trapezoide (x-Achsen-Abschnittchen */
    node_local_n = n / nodes;    /* Anzahl der Knoten-lokal zu berechnenden Trapezoide */
    node_local_a = a + node * node_local_n * h; /* untere Integrationsgrenze je Knoten */
    node_local_b = node_local_a + node_local_n * h; /* obere Integrationsgrenze je Knoten */

    
    /* Knoten-lokale Threads erzeugen */
    for (i = 0; i < local_t; i++){ /* Rechen-Threads erzeugen */
#ifdef DEBUG
        printf("Knoten %d: Beginne mit Setzen der Argumente für Thread %d\n", node, i);
#endif
        args_t *args;
        args = malloc(sizeof(args_t));
        args_array[i] = args;
        args->funktion = fktn_f;
        /* Thread-lokale Grenzen berechnen: */
        thread_local_n = node_local_n / local_t;
        thread_local_a = node_local_a + i * thread_local_n * h; /* linke Grenze je Thread */
        thread_local_b = node_local_a + node_local_n * h; /* ToDo: Aufteilen -> rechte Grenze je Thread */

        args->thread_local_a = thread_local_a;
        args->thread_local_b = thread_local_b;
        args->thread_local_n = thread_local_n;
        args->thread_local_integral = 0.0;
        /* Threads erzeugen */
#ifdef DEBUG
        printf("Knoten %d: Starte Thread %d\n",node, i);
#endif
        thread_status = pthread_create(&threads[i], NULL, rechne, (void*)args);
        if(thread_status != 0){
            fprintf(stderr, "Error: Thread konnte nicht erzeugt werden\n");
            exit(EXIT_FAILURE);
        }
    }

    /* jetzt Threads joinen und auf Ende warten */
    for (i = 0; i < local_t; i++){
        thread_status = pthread_join(threads[i], &thread_rc);
        if(thread_status != 0){
            fprintf(stderr, "Error: Thread konnte nicht gejoined werden\n");
            exit(EXIT_FAILURE);
        }
        node_local_integral += *(long double*)thread_rc; /* Teilergebnisse zusammenfügen */
#ifdef DEBUG
        printf("Knoten %d: Thread %d meldet Teilergebnis: %Lf\n", node, i, node_local_integral);
#endif
    }

    /* Die Ergebnisse aus allen Knoten zusammenfassen */
    MPI_Reduce(&node_local_integral,    /* sende das lokale Integral */
               &integral,               /* für das Gesamtintegral */
               1,                       /* jeweils ein Datum */
               MPI_LONG_DOUBLE,         /* vom Typ long double */
               MPI_SUM,                 /* Aggregatfunktion: Summiere */
               0,                       /* Empfänger: Knoten 0 */
               MPI_COMM_WORLD);         /* Sender: Alle beteiligten Knoten */

    if(node == 0){
        ende = clock();
        dauer = (((long double)ende -(long double)start)/((long double)CLOCKS_PER_SEC))*(long double)1000;
        printf("Dauer der Berechnung mit %ld Trapezoiden mit %d Threads: %.2Lf Millisekunden\n",
                n,total_t,dauer);
        printf("Das Integral von f(x)=x^2 von %Lf bis %Lf ist: %Le\n",
                a, b, integral);
    }
    free(threads);
    free(args_array);
    MPI_Finalize(); /* Cluster wieder aufräumen */
    exit(EXIT_SUCCESS);
}
