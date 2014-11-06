/******************************************************
 * Berechnung von Integralen mittels der Trapezregel
 *
 * Die drei Funktionen werden der Einfachheit halber
 * fest einprogrammiert. Vorgesehen ist f(x)=x^2 und
 * f(x) = 4/(1+x^2), da die einfach analytisch zu lösen
 * sind und ich die Rechenergebnisse prüfen kann :)
 * Zusätzlich noch f(x) = x/(1+e^x), welche analytisch
 * nicht lösbar sein sollte.
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
#include "fktn_f.h"         /* x^2 */
#include "fktn_g.h"         /* 4/(1+x^2) */
#include "fktn_h.h"         /* sin(x)*e^x */
#include "identify_myself.h"
#include "rechne.h"
#include "mpi.h"

#define MSG_TAG_THR     42  /* Tag für die Abfrage von Threadanzahl der Knoten */

int main(int argc, char **argv){
    int nodes;              /* Anzahl der Knoten */
    int node;               /* lokale Knotennummer */
    int local_threads = 0;        /* Anzahl der Threads (lokal je Knoten) */
    int total_threads = 0;        /* Gesamtzahl der verwendeten Threads */
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
    clock_t f_start = 0.0;  /* Startzeiten je zu berechnender Funktion */
    clock_t ende = 0.0;     /* Endezeit */
    clock_t f_ende = 0.0;   /* Endezeit je zu berechnender Funktion */
    long double dauer = 0.0;  /* Dauer des Programmlaufs */
    long double integral = 0.0;   /* Das Gesamtergebnis */
    long double node_local_integral=0.0; /* Ergebnis je Knoten */
    int i = 0;              /* Zaehler */
    int j = 0;              /* noch ein Zaehler */
    int c = 0;              /* Rückgabewert von getopt */
    opterr = 0;             /* Error-Var */
    int thread_status;      /* Status der pthread-Funktionen */
    pthread_t *threads;     /* Speichert die Thread-ID's */
    args_t *args;           /* Zeiger auf Argumente, die an die Threads übergeben werden */
    args_t **args_arr;      /* Array von Zeigern auf die Zeiger auf die Argumente */
    args_t ***pargs_arr;    /* Zeiger auf das aktuelle Array-Element */
    long int n_quer;        /* Stückchen des Integrals */
    MPI_Status status;      /* Nachrichtenstatus */
    int *tmp_threads;            /* Vektor, der die Threads aller Knoten speichert */

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
        case AMD_OCTA_CORE: local_threads = 8*6; /* 8 Kerne, 6-fache Taktfrequenz eines Pi */
                            break;
        case RASPBERRY_PI:  local_threads = 1;
                            break;
        case BANANA_PI:     local_threads = 2;
                            break;
        default:            local_threads = 1;
    }
    /* Commandline Parameter auswerten */
    while((c = getopt(argc, argv, "t:a:b:n:")) != -1)
        switch(c){
            case 't':   local_threads = atoi(optarg);
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
        printf("t = %d, a = %Lf, b = %Lf, n = %ld, World = %d\n", local_threads,a,b,n,nodes);
#endif

    /* Zeitmessung starten */
    if(node == 0)
        start = clock();

    /* Da jeder einzelne Knoten eine unterschiedliche Anzahl an Threads besitzen kann, lässt sich
     * die jeweilige lokale linke und rechte Grenze nicht einfach errechnen, ohne dass die Knoten
     * Kenntnis über die komplette Clusterstruktur haben. -> Knoten 0 muss alle Knoten abfragen,
     * wie viele Threads sie bedienen und dann das Ergebnis durchs Netz propagieren */
    tmp_threads = malloc(sizeof(int) * (nodes+1));
    if (tmp_threads ==NULL){
        perror("Fehler bei der Speicherallokation - tmp_threads\n");
        exit(EXIT_FAILURE);
    }

    if(node == 0){
        tmp_threads[0] = local_threads;         /* Anzahl möglicher Threads bei Knoten 0 */
        total_threads += local_threads;
        /* die anderen Knoten melden ihre Threadmöglichkeiten: */
        for(i = 1; i < nodes; i++){
            MPI_Recv(&tmp_threads[i],/* Wohin wird der zu empfangende Wert gespeichert */
                    1,             /* einzelner Integerwert */
                    MPI_INT,       /* Integer */
                    i,             /* Source -> die anderen Knoten */
                    MSG_TAG_THR,   /* Message-Tag */
                    MPI_COMM_WORLD,/* Das komplette Cluster */
                    &status);      /* Status der Mitteilung */
            total_threads += tmp_threads[i];    /* Aufsummieren, um die Gesamtkapazität zu erhalten */
        }
        tmp_threads[nodes] = total_threads;   /* Die Gesamtzahl an Threads zur Übergabe speichern */
    }else {                         /* alle anderen Knoten senden ihre Info */
        MPI_Send(&local_threads,    /* die Threads, die je Knoten zur Verfügung stehen */
                1,                 /* ist ein Integer */
                MPI_INT,           /* Datentyp: Int */
                0,                 /* Nachrichtenziel: Knoten 0 */
                MSG_TAG_THR,       /* Message-Tag als Identifier */
                MPI_COMM_WORLD);   /* Das komplette Cluster */
    }

    /* Nun die Strukturinfo des Clusters propagieren, damit jeder Knoten seinen
     * Arbeitsbereich errechnen kann. MPI_Bcast sorgt dabei für einen 
     * implementationsspezifisch-optimierten Kommunikationsweg; i.d.R. in Form
     * einer Baumstruktur, so dass nur Log2(n) Nachrichten gesendet werden müssen. */

    MPI_Bcast(tmp_threads,      /* Beginn des Arrays */
            nodes+1,          /* Anzahl der enthaltenen Daten */
            MPI_INT,          /* vom Typ Integer */
            0,                /* Quelle */
            MPI_COMM_WORLD);  /* Ziel: Das komplette Cluster */

    /* Ab hier rechnet jeder Clusterknoten seinen Arbeitsbereich aus:
     * Damit jeder Knoten gleichviel zu tun hat: Trapezoidanzahl hoch rechnen, so 
     * dass n ohne Rest durch nodes teilbar ist */
    total_threads = tmp_threads[nodes]; /* War oben nur lokal genutzt */
    n_quer = nodes * total_threads;
    if((n % n_quer) != 0)
        n = ((n / n_quer)+1) * n_quer;
    n_quer = n / total_threads;         /* Trapezoide je Thread */
    h = (b-a)/n;                        /* Breite (Basis) eines jeden Trapezoids */
    node_local_n = n_quer /* n pro thread */
        * local_threads;            /* lokale Threads = n pro Knoten */
    /* Iterativ die Obergrenze des vorherigen Knoten berechnen, das ist die
     * Untergrenze des aktuellen Knotens */
    node_local_a = a;
    for (i = 0; i < node; i++){     /* Zählen, wieviele Threads bereits Arbeit haben */
        node_local_a += (long double)n_quer *
            (long double)tmp_threads[i] * h; /* das wurde bereits verarbeitet */
    }
    node_local_b = node_local_a +   /* Obergrenze = Untergrenze + */
        node_local_n * h;           /* Anzahl der Trapezoide * Basisbreite */

    free(tmp_threads);              /* Wird nicht mehr gebraucht */
    /* Speicher für Threads allokieren */
    threads = malloc(sizeof(pthread_t)*local_threads);
    if (threads == NULL){
        perror("Fehler bei der Speicherallokation - threads\n");
        exit(EXIT_FAILURE);
    }

    /* Speicher für das ArgumenteZeigerArray reservieren */
    args_arr = malloc(local_threads * sizeof(args_t*));
    if (args_arr == NULL){
        perror("Fehler bei der Speicherallokation - args_arr\n");
        exit(EXIT_FAILURE);
    }

    for(j = 0; j < 3; j++){ /* drei  Funktionen durchrechnen */
        f_start = clock();
        pargs_arr = &args_arr;
        /* Knoten-lokale Threads erzeugen */
        for (i = 0; i < local_threads; i++){ /* Rechen-Threads erzeugen */
            /* Speicher für Thread-Argumente reservieren */
            args = malloc(sizeof(args_t));
            (*pargs_arr)[i] = args;
            switch(j){
                case 0: args->funktion = fktn_f;
                        break;
                case 1: args->funktion = fktn_g;
                        break;
                case 2: args->funktion = fktn_h;
            }
            /* Thread-lokale Grenzen berechnen: */
            thread_local_n = node_local_n / local_threads;
            thread_local_a = node_local_a + i * thread_local_n * h; /* linke Grenze je Thread */
            thread_local_b = thread_local_a + thread_local_n * h; /* ToDo: Aufteilen -> rechte Grenze je Thread */
            args->thread_local_a = thread_local_a;
            args->thread_local_b = thread_local_b;
            args->thread_local_n = thread_local_n;
            args->thread_local_integral = 0.0;
#ifdef DEBUG
            printf("Knoten %d, Thread %d, n = %ld, a = %Lf, b = %Lf\n", node, i, thread_local_n, thread_local_a, thread_local_b);
#endif
            /* Threads erzeugen */
            thread_status = pthread_create(&threads[i], NULL, rechne, (void*)args);
            if(thread_status != 0){
                perror("Error: Thread konnte nicht erzeugt werden\n");
                exit(EXIT_FAILURE);
            }
        }

        /* jetzt Threads joinen und auf Ende warten */
        for (i = 0; i < local_threads; i++){
            thread_status = pthread_join(threads[i], NULL);
            if(thread_status != 0){
                perror("Error: Thread konnte nicht gejoined werden\n");
                exit(EXIT_FAILURE);
            }
            node_local_integral += (*pargs_arr)[i]->thread_local_integral; /* Teilergebnisse zusammenfügen */
#ifdef DEBUG
            printf("Knoten: %d, Funktion: %d,  Teilintegral: %Lf\n",node,j, node_local_integral);
#endif
            free((*pargs_arr)[i]); /* das ehemealige args */
        }
#ifdef DEBUG
        printf("Knoten: %d, Teilintegral: %Lf\n", node, node_local_integral);
#endif
        if(nodes == 1){
            integral = node_local_integral;
        } else {
        /* Die Ergebnisse aus allen Knoten zusammenfassen */
        MPI_Reduce(&node_local_integral,    /* sende das lokale Integral */
                &integral,               /* für das Gesamtintegral */
                1,                       /* jeweils ein Datum */
                MPI_LONG_DOUBLE,         /* vom Typ long double */
                MPI_SUM,                 /* Aggregatfunktion: Summiere */
                0,                       /* Empfänger: Knoten 0 */
                MPI_COMM_WORLD);         /* Sender: Alle beteiligten Knoten */
        }
        if(node == 0){
            f_ende = clock();
            dauer = (((long double)f_ende -(long double)f_start)/((long double)CLOCKS_PER_SEC))*(long double)1000;
            switch(j){
                case 0: printf("Dauer der Berechnung des Integrals von f(x) = x^2\n");
                        printf("durch %ld Trapezoide mit %d Threads auf %d Knoten: %.2Lf Millisekunden\n",
                                n,total_threads,nodes,dauer);
                        printf("Das Integral von f(x)=x^2 von %Lf bis %Lf ist: %.16Lf\n\n",
                                a, b, integral);
                        integral = 0.0;
                        break;
                case 1: printf("Dauer der Berechnung des Integrals von f(x) = 4/(1+x^2)\n");
                        printf("durch %ld Trapezoide mit %d Threads auf %d Knoten: %.2Lf Millisekunden\n",
                                n,total_threads,nodes,dauer);
                        printf("Das Integral von f(x)=4/(1+x^2) von %Lf bis %Lf ist: %.16Lf \n\n",
                                a, b, integral);
                        integral = 0.0;
                        break;
                case 2: printf("Dauer der Berechnung des Integrals von f(x) = sin(x)*e^x\n");
                        printf("durch %ld Trapezoide mit %d Threads auf %d Knoten: %.2Lf Millisekunden\n",
                                n,total_threads,nodes,dauer);
                        printf("Das Integral von f(x)=sin(x)*e^x von %Lf bis %Lf ist: %.16Lf\n\n",
                                a, b, integral);
                        integral = 0.0;
                        break;
            }
            node_local_integral = 0.0;
            fflush(NULL);
            sleep(2);

        }
    }
    ende = clock();
    dauer = (((long double)ende-(long double)start)/((long double)CLOCKS_PER_SEC))*(long double)1000;
    if(node == 0)
        printf("Die gesamten Berechnungen dauerten mit %d Threads auf %d Knoten: %.2Lf Millisekunden\n",
                total_threads, nodes, dauer);

    free(threads);
    free(args_arr);
    MPI_Finalize(); /* Cluster wieder aufräumen */
    exit(EXIT_SUCCESS);
}
