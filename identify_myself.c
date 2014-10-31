//           identify_myself.cxx
//  Di Oktober 28 11:47:02 2014
//  Copyright  2014  Cluster-Controller
//  <deppenmagnet@openmailbox.org>
// Copyright (C) 2014  Cluster-Controller
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with main.c; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA


#include "identify_myself.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void identify_myself(int* node_type){
	/* die Datei /proc/cpuinfo einlesen */
	FILE *fp;
	/* Puffer erstellen */
	char buffer[10*1024];
	/* Anzahl der gelesenen Zeichen speichern */
	size_t bytes_read;
	/* gesuchte Zeichenkette: */
	char *match;
	char *model;

	/* zuerst Datei öffnen, read-only */
	if((fp = fopen("/proc/cpuinfo", "r")) == NULL){
		/* Fehler */
		perror("fopen() auf /proc/cpuinfo");
		exit(EXIT_FAILURE);
	}
	/* auf einen Rutsch einlesen */
	bytes_read = fread(buffer, 1, sizeof(buffer), fp);
	/* Filepointer (handle) schließen */
	fclose(fp);
	if (bytes_read == 0 || bytes_read == sizeof(buffer)){
		perror("fopen() - irgendwas stimmt nicht!");
		exit(EXIT_FAILURE);
	}
	buffer[bytes_read] = '\0'; /* Stringende anhängen */

	/* Jetzt nach der Zeichenfolge cpu family suchen */
	match = strstr(buffer, "model name");
	if (match == 0){ /* nicht gefunden... */
		*node_type = NO_CPU_INFO;
		return;
	}
	if((model = strstr(buffer, "AMD")) != NULL){
		*node_type = AMD_OCTA_CORE;
    } else if ((model = strstr(buffer, "ARMv6")) != NULL){
        *node_type = RASPBERRY_PI;
    } else if ((model = strstr(buffer, "ARMv7")) != NULL){
        *node_type = BANANA_PI;
    } else {
        *node_type = NO_CPU_INFO;
    }
#ifdef DEBUG
    printf("Erkannter Prozessor: %d\n", *node_type);
#endif
}
