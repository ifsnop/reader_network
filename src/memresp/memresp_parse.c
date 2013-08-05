/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2012 Diego Torres <diego dot torres at gmail dot com>

This file is part of the reader_network utils.

reader_network is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

reader_network is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with reader_network. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>	       
#include <errno.h>
#include <string.h>
#include <math.h>

#define BLOCK_READ 	0x48
#define MODO_A 2
#define MODO_C 1
#define MODO_UNDEF 0
#define TIPO_FALSA 0
#define TIPO_BUENA 1
#define MASK_TIPO 	0x0008
#define MAX_TRACKER 100
#define INIT_PLOT 3

void printHex(unsigned char *p, int c) {
int i;

    for(i=0;i<c;i++) {
	if (i!=0 && i%8==0) { printf(" "); }
	if (i!=0 && i%16==0) { printf("\n"); }
	printf("%02X ", p[i]);
    }
    printf("\n");
    return;
}

struct mr {
    unsigned char f1_vep1, f1_vn1, f1_vep2, f1_vn2;
    unsigned char c1_vep1, c1_vn1, c1_vep2, c1_vn2;
    unsigned char a1_vep1, a1_vn1, a1_vep2, a1_vn2;
    unsigned char c2_vep1, c2_vn1, c2_vep2, c2_vn2;
    unsigned char a2_vep1, a2_vn1, a2_vep2, a2_vn2;
    unsigned char c4_vep1, c4_vn1, c4_vep2, c4_vn2;
    unsigned char a4_vep1, a4_vn1, a4_vep2, a4_vn2;
    unsigned char x_vep1, x_vn1, x_vep2, x_vn2;
    unsigned char b1_vep1, b1_vn1, b1_vep2, b1_vn2;
    unsigned char d1_vep1, d1_vn1, d1_vep2, d1_vn2;
    unsigned char b2_vep1, b2_vn1, b2_vep2, b2_vn2;
    unsigned char d2_vep1, d2_vn1, d2_vep2, d2_vn2;
    unsigned char b4_vep1, b4_vn1, b4_vep2, b4_vn2;
    unsigned char d4_vep1, d4_vn1, d4_vep2, d4_vn2;
    unsigned char f2_vep1, f2_vn1, f2_vep2, f2_vn2;
    unsigned char spi_vep1, spi_vn1, spi_vep2, spi_vn2;
    unsigned char dist[2], azimuth[2], flag_pos[2], flag_ant[2];    
    int resp;
    int modo;
    int tipo;
    float mr_dist, mr_azimuth;
    int mr_flag_pos, mr_flag_ant;
    struct mr *next;
};

#define EPSILON_AZIMUTH 0.6
#define EPSILON_DIST 0.04


struct mr **tracker;
int usage[MAX_TRACKER];
int current_usage = 0;
float current_azimuth = 0;

void purgeTracker() {
    int i=0, j=0;
    while (j<current_usage) {
	if (usage[i] != 0) {
	    struct mr *p = tracker[i];
	    float diff_azimuth = 0.0;
	    int count = 0;
	    while(p->next!=NULL) {p = p->next; count++; }
	    diff_azimuth = fabs(p->mr_azimuth - current_azimuth);
	    if (diff_azimuth>EPSILON_AZIMUTH*2) {
		p = tracker[i];
		printf("eliminando %d resp(%d) con diff azimuth(%3.3f) current_azimuth(%3.3f)\n", i, p->resp, diff_azimuth, current_azimuth);
		if (count>INIT_PLOT) { printf("PLOT count(%d)\n", count); }
		while(p!=NULL) {
		    struct mr *tmp = p;
		    p=p->next;
		    free(tmp);
		}
		current_usage--;
		usage[i] = 0;
		tracker[i] = NULL;
	    }
	    j++;
	}
	i++;
    }
    return;
}
void printTracker() {
    int i=0, j=0;
    while (j<current_usage) {
	if ( usage[i] != 0 ) {
	    struct mr *p = tracker[i];
	    printf("%d ", i);
	    while(p!=NULL) {
		printf("%d)%3.3f %3.3f|", p->resp, p->mr_dist, p->mr_azimuth);
		p = p->next;
	    }
	    printf("\n");
	    j++;
	}
	i++;
    }
    return;
}


int getFree() {
    int i=0;
    while ( (usage[i]!=0) && (i<MAX_TRACKER) ) i++;
    if (i==100) return -1;
    return i;
}

int main(int argc, char *argv[]) {
int fd, readed_bytes, resp = 0;
struct mr *p;
struct mr *blank;

    
    if (argc!=2) { printf("memresp_parse record.bin\n"); exit(EXIT_FAILURE); }
    printf("%s\n", argv[1]);

    if ( (fd = open(argv[1], O_RDONLY))==-1 ) { printf("error open: %s\n", strerror(errno)); exit(EXIT_FAILURE); }

    p = (struct mr *) malloc(sizeof(struct mr)); memset(p, 0, sizeof(struct mr));
    blank = (struct mr *) malloc(sizeof(struct mr)); memset(blank, 0, sizeof(struct mr));    
    memset(usage, 0, sizeof(int [100]));
    tracker = (struct mr **)malloc(sizeof(struct mr)*MAX_TRACKER); memset(tracker, 0, sizeof(struct mr)*MAX_TRACKER);
    
    while( (readed_bytes = read(fd, (void *) p, BLOCK_READ)) == BLOCK_READ ) {
	int free_ptr = 0, i=0, j=0;
	resp++;
	if (resp>170) break;
//	if (!memcmp(p,blank,BLOCK_READ-8)) { continue; }

//	printf("RESP: %d in %08X\n", resp, p);
//	printHex((unsigned char *)p, BLOCK_READ);
	p->resp = resp;
	p->mr_dist = ((p->dist[0]<<8) + p->dist[1])*50.0/1000.0*299.792458/1852.0/2.0; //0.080937402277284182256314037120836; // *300.0/1852.0/2.0;
	p->mr_azimuth = current_azimuth = (((p->azimuth[0]<<8) + p->azimuth[1]) & 0x1FFF)*360.0/16384.0;
	p->mr_flag_pos = (p->flag_pos[0]<<8) + p->flag_pos[1];
	p->mr_flag_ant = (p->flag_ant[0]<<8) + p->flag_ant[1];
	
	if ( (p->mr_flag_pos & 0x0F00) == 0x0D00 ) { p->modo = MODO_C; /*printf("modoC\n");*/ }
	if ( (p->mr_flag_pos & 0x0F00) == 0x0B00 ) { p->modo = MODO_A; /*printf("modoA\n");*/ }
	if ( (p->mr_flag_ant & MASK_TIPO) == MASK_TIPO ) { p->tipo = TIPO_BUENA; }
	
	// comprobar si hay que cancelar algun plot en formacion

	purgeTracker();
	
	if (p->tipo == TIPO_FALSA) { continue; } // si es respuesta falsa no la tratamos

	while(j<current_usage) {
	    if ( usage[i] != 0 ) { // esta ocupado, comprobamos si entra en ventana
		float diff_azimuth,diff_dist;
		struct mr *t = tracker[i];
		while (t->next!=NULL) { t=t->next; } // avanzamos hasta la ultima respuesta
		
		diff_azimuth = fabs( (t->mr_azimuth) - (p->mr_azimuth) );
		diff_dist = fabs( (t->mr_dist) - (p->mr_dist) );
	        
		if ( (diff_azimuth < EPSILON_AZIMUTH) && (diff_dist < EPSILON_DIST) ) {
		    // añadir respuesta al plot actual
		    //struct mr *t = tracker[i];
		    //printf("insertando resp(%d)%08X en %08X: ", p->resp, (unsigned int)p, (unsigned int)t);
		    //while(t->next != NULL) { printf("%08X %08X ", (unsigned int)t, (unsigned int)t->next); t = t->next; }
		    //printf("in %08X\n", (unsigned int)t);
		    t->next = (struct mr *) malloc(sizeof(struct mr)); memset(t->next, 0, sizeof(struct mr));
		    memcpy(t->next, p, sizeof(struct mr));
		    goto next;
		}
		j++; //ya hemos mirado un elemento mas 
	    }
	    i++;
	}
	if (j == current_usage) { // no se ha podido asociar, crear nuevo plot
	    if ( (free_ptr = getFree()) == -1 ) { printf("error outoftracker resources\n"); exit(EXIT_FAILURE); }
	    printf("generando nuevo plot en %d con resp %d, azimuth actual(%3.3f)\n", free_ptr, p->resp, current_azimuth);
	    tracker[free_ptr] = (struct mr *) malloc(sizeof(struct mr)); memset(tracker[free_ptr], 0, sizeof(struct mr));
	    memcpy(tracker[free_ptr], p, sizeof(struct mr));
	    current_usage++;
	    usage[free_ptr] = 1;
	}
    
next:
	
/*
	printf("%d, "
	    "%d,%d,%d,%d, "
	    "%d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, "
	    "%d,%d,%d,%d, "
	    "%d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, %d,%d,%d,%d, "
	    "%d,%d,%d,%d, "
	    "%d,%d,%d,%d, %d, %2.4f, %2.4f, %02X%02X, %02X%02X\n",
	    p->resp, 
	    p->f1_vep1, p->f1_vn1, p->f1_vep2, p->f1_vn2,
	    p->c1_vep1, p->c1_vn1, p->c1_vep2, p->c1_vn2,
	    p->a1_vep1, p->a1_vn1, p->a1_vep2, p->a1_vn2,
	    p->c2_vep1, p->c2_vn1, p->c2_vep2, p->c2_vn2,
	    p->a2_vep1, p->a2_vn1, p->a2_vep2, p->a2_vn2,
	    p->c4_vep1, p->c4_vn1, p->c4_vep2, p->c4_vn2,
	    p->a4_vep1, p->a4_vn1, p->a4_vep2, p->a4_vn2,
	    p->x_vep1,  p->x_vn1,  p->x_vep2,  p->x_vn2,
	    p->b1_vep1, p->b1_vn1, p->b1_vep2, p->b1_vn2,
	    p->d1_vep1, p->d1_vn1, p->d1_vep2, p->d1_vn2,
	    p->b2_vep1, p->b2_vn1, p->b2_vep2, p->b2_vn2,
	    p->d2_vep1, p->d2_vn1, p->d2_vep2, p->d2_vn2,
	    p->b4_vep1, p->b4_vn1, p->b4_vep2, p->b4_vn2,
	    p->d4_vep1, p->d4_vn1, p->d4_vep2, p->d4_vn2,
	    p->f2_vep1, p->f2_vn1, p->f2_vep2, p->f2_vn2,
	    p->spi_vep1,p->spi_vn1,p->spi_vep2,p->spi_vn2,
	    p->modo, p->mr_dist, p->mr_azimuth, p->flag_pos[0], p->flag_pos[1], p->flag_ant[0], p->flag_ant[1]);
*/
	
//	printf("F1(%d) F2(%d)\n", p->f1_vep1, p->f2_vep1);
//	printf("%3.3f %3.3f\n", p->mr_dist, p->mr_azimuth);
	

//	printf("%02X%02X %02X%02X\n", p->flag_pos[0], p->flag_pos[1], p->flag_ant[0], p->flag_ant[1]);

//	exit(EXIT_FAILURE);


	memset(p, 0, sizeof(struct mr));	
	
	printTracker();
	
	
    }
    printf("FIN(%d)\n", readed_bytes);
    
    printTracker();
    
    exit(EXIT_SUCCESS);
}

