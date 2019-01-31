/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2019 Diego Torres <diego dot torres at gmail dot com>

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

#include "includes.h"

#define SERVER_TIMEOUT_SEC 10

struct sorted_list {
    float segment;
    int count;
    struct sorted_list *next;
};

struct radar_delay_s {
    unsigned char sac,sic;
    long cuenta_plot_cat1, cuenta_plot_cat2;
    long cuenta_plot_cat8, cuenta_plot_cat10;
    long cuenta_plot_cat19, cuenta_plot_cat20;
    long cuenta_plot_cat21;
    long cuenta_plot_cat34, cuenta_plot_cat48;
    float suma_retardos_cat1, suma_retardos_cat2;
    float suma_retardos_cat8, suma_retardos_cat10;
    float suma_retardos_cat19, suma_retardos_cat20;
    float suma_retardos_cat21;
    float suma_retardos_cat34, suma_retardos_cat48;
    float suma_retardos_cuad_cat1, suma_retardos_cuad_cat2;
    float suma_retardos_cuad_cat8, suma_retardos_cuad_cat10;
    float suma_retardos_cuad_cat19, suma_retardos_cuad_cat20;
    float suma_retardos_cuad_cat21;
    float suma_retardos_cuad_cat34, suma_retardos_cuad_cat48;
    float max_retardo_cat1, max_retardo_cat2;
    float max_retardo_cat8, max_retardo_cat10;
    float max_retardo_cat19, max_retardo_cat20;
    float max_retardo_cat21;
    float max_retardo_cat34, max_retardo_cat48;
    float min_retardo_cat1, min_retardo_cat2;
    float min_retardo_cat8, min_retardo_cat10;
    float min_retardo_cat19, min_retardo_cat20;
    float min_retardo_cat21;
    float min_retardo_cat34, min_retardo_cat48;
    int *segmentos_cat1, *segmentos_cat2;
    int *segmentos_cat8, *segmentos_cat10;
    int *segmentos_cat19, *segmentos_cat20;
    int *segmentos_cat21;
    int *segmentos_cat34, *segmentos_cat48;
    int segmentos_max_cat1, segmentos_max_cat2;
    int segmentos_max_cat8, segmentos_max_cat10;
    int segmentos_max_cat19, segmentos_max_cat20;
    int segmentos_max_cat21;
    int segmentos_max_cat34, segmentos_max_cat48;
    int segmentos_ptr_cat1, segmentos_ptr_cat2;
    int segmentos_ptr_cat8, segmentos_ptr_cat10;
    int segmentos_ptr_cat19, segmentos_ptr_cat20;
    int segmentos_ptr_cat21;
    int segmentos_ptr_cat34, segmentos_ptr_cat48;
    struct sorted_list *sorted_list_cat1, *sorted_list_cat2;
    struct sorted_list *sorted_list_cat8, *sorted_list_cat10;
    struct sorted_list *sorted_list_cat19, *sorted_list_cat20;
    struct sorted_list *sorted_list_cat21;
    struct sorted_list *sorted_list_cat34, *sorted_list_cat48;
};

struct radar_delay_s *radar_delay;

struct ip_mreq mreq;
struct sockaddr_in addr;
fd_set reader_set;
int s, yes = 1;
bool forced_exit = false;

void insertList(struct sorted_list **p, int segment, int count) {

    float fsegment = (segment * 0.005) - 8;
//    log_printf(LOG_NORMAL, "0) insertando(%d) ROOT(%08X)\n", count, (unsigned int)*p);
    if (*p==NULL) {
	*p = (struct sorted_list *) mem_alloc(sizeof(struct sorted_list));
	(*p)->segment = fsegment;
	(*p)->count = count;
	(*p)->next = NULL;
//	log_printf(LOG_NORMAL, "1)%d ROOT(%08X)\n", (*p)->count, (unsigned int)(*p));
    } else { // ordenaremos de menor a mayor
	struct sorted_list *t = *p;
	struct sorted_list *nuevo;
	struct sorted_list *old = NULL;

//	log_printf(LOG_NORMAL, "2)%d %08X\n", t->count, (unsigned int)t);
	
//	int i=0;
	while((fsegment > t->segment) && (t->next!=NULL)) {
//	    log_printf(LOG_NORMAL, "%d\n", i++);
	    old = t;
	    t = t->next;
	}
//        log_printf(LOG_NORMAL, "3)ROOT(%08X) t(%08X)\n",(unsigned int) *p, (unsigned int) t);

	nuevo = (struct sorted_list *) mem_alloc(sizeof(struct sorted_list));
	nuevo->count = count;
	nuevo->segment = fsegment;
	nuevo->next = NULL;

	if (fsegment > t->segment) { // se da de alta detras del elemento actual
	    nuevo->next = t->next;
	    t->next = nuevo;
//	    log_printf(LOG_NORMAL, "4)\n");
	} else {
	    if (fsegment <= t->segment && (t!=*p)) { // se da de alta en lugar del elemento actual, pero no es ppio de lista
		old->next = nuevo;
		nuevo->next = t;
//	        log_printf(LOG_NORMAL, "5)\n");
	    }
	    if (t == *p) { // insertando al principio de la lista
		nuevo->next = *p; 
		*p = nuevo;
//		log_printf(LOG_NORMAL, "6)\n");
	    }
	}
//	log_printf(LOG_NORMAL, "7)%d %08X\n", nuevo->count, (unsigned int)nuevo);
    }
//    log_printf(LOG_NORMAL, "8)ROOT(%08X)\n\n", (unsigned int)*p);
    return;
}

void server_connect(void) {

    FD_ZERO(&reader_set);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PLOTS_PORT);
    if ( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	log_printf(LOG_ERROR, "socket %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    FD_SET(s, &reader_set);

    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
	log_printf(LOG_ERROR, "setsockopt %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    if ( bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	log_printf(LOG_ERROR, "bind %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
#if defined(__sun)
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
#endif
#if defined(__linux)
    mreq.imr_interface.s_addr = inet_addr("127.0.0.1"); //nHostAddress;
#endif
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_PLOTS_GROUP);
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
	log_printf(LOG_ERROR, "setsocktperror\n");
	exit(EXIT_FAILURE);
    }

    return;
}

void radar_delay_alloc(void) {
int i;

    radar_delay = (struct radar_delay_s *) mem_alloc(sizeof(struct radar_delay_s)*MAX_RADAR_NUMBER);
    for (i=0; i < MAX_RADAR_NUMBER; i++) {
	radar_delay[i].segmentos_cat1 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat2 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat8 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat10 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat19 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat20 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat21 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat34 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].segmentos_cat48 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
	radar_delay[i].sorted_list_cat1 = radar_delay[i].sorted_list_cat2 = NULL;
	radar_delay[i].sorted_list_cat8 = radar_delay[i].sorted_list_cat10 = NULL;
	radar_delay[i].sorted_list_cat19 = radar_delay[i].sorted_list_cat20 = NULL;
	radar_delay[i].sorted_list_cat21 = NULL;
	radar_delay[i].sorted_list_cat34 = radar_delay[i].sorted_list_cat48 = NULL;
    }
    return;
}

void radar_delay_clear(void) {
int i;
    for (i=0; i < MAX_RADAR_NUMBER; i++) {
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat1;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat2;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat8;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat10;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat19;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat20;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat21;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat34;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	{
	    struct sorted_list *p = radar_delay[i].sorted_list_cat48;
	    while (p!=NULL) { struct sorted_list *p2 = p->next; mem_free(p); p = p2; }
	}
	radar_delay[i].sorted_list_cat1 = radar_delay[i].sorted_list_cat2 = NULL;
	radar_delay[i].sorted_list_cat8 = radar_delay[i].sorted_list_cat10 = NULL;
	radar_delay[i].sorted_list_cat19 = radar_delay[i].sorted_list_cat20 = NULL;
	radar_delay[i].sorted_list_cat21 = NULL;
	radar_delay[i].sorted_list_cat34 = radar_delay[i].sorted_list_cat48 = NULL;

	memset(radar_delay[i].segmentos_cat1, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat2, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat8, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat10, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat19, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat20, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat21, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat34, 0, sizeof(int) * MAX_SEGMENT_NUMBER);
	memset(radar_delay[i].segmentos_cat48, 0, sizeof(int) * MAX_SEGMENT_NUMBER);

	radar_delay[i].sac = '\0'; radar_delay[i].sic = '\0';
	radar_delay[i].cuenta_plot_cat1 = 0; radar_delay[i].cuenta_plot_cat2 = 0;
	radar_delay[i].cuenta_plot_cat8 = 0; radar_delay[i].cuenta_plot_cat10 = 0;
	radar_delay[i].cuenta_plot_cat19 = 0; radar_delay[i].cuenta_plot_cat20 = 0;
	radar_delay[i].cuenta_plot_cat21 = 0;
	radar_delay[i].cuenta_plot_cat34 = 0; radar_delay[i].cuenta_plot_cat48 = 0;
	radar_delay[i].suma_retardos_cat1 = 0; radar_delay[i].suma_retardos_cat2 = 0;
	radar_delay[i].suma_retardos_cat8 = 0; radar_delay[i].suma_retardos_cat10 = 0;
	radar_delay[i].suma_retardos_cat19 = 0; radar_delay[i].suma_retardos_cat20 = 0;
	radar_delay[i].suma_retardos_cat21 = 0;
	radar_delay[i].suma_retardos_cat34 = 0; radar_delay[i].suma_retardos_cat48 = 0;
	radar_delay[i].suma_retardos_cuad_cat1 = 0; radar_delay[i].suma_retardos_cuad_cat2 = 0;
	radar_delay[i].suma_retardos_cuad_cat8 = 0; radar_delay[i].suma_retardos_cuad_cat10 = 0;
	radar_delay[i].suma_retardos_cuad_cat19 = 0; radar_delay[i].suma_retardos_cuad_cat20 = 0;
	radar_delay[i].suma_retardos_cuad_cat21 = 0;
	radar_delay[i].suma_retardos_cuad_cat34 = 0; radar_delay[i].suma_retardos_cuad_cat48 = 0;
	radar_delay[i].max_retardo_cat1 = -10000.0; radar_delay[i].max_retardo_cat2 = -10000.0;
	radar_delay[i].max_retardo_cat8 = -10000.0; radar_delay[i].max_retardo_cat10 = -10000.0;
	radar_delay[i].max_retardo_cat19 = -10000.0; radar_delay[i].max_retardo_cat20 = -10000.0;
	radar_delay[i].max_retardo_cat21 = -10000.0;
	radar_delay[i].max_retardo_cat34 = -10000.0; radar_delay[i].max_retardo_cat48 = -10000.0;
	radar_delay[i].min_retardo_cat1 = 10000.0; radar_delay[i].min_retardo_cat2 = 10000.0;
	radar_delay[i].min_retardo_cat8 = 10000.0; radar_delay[i].min_retardo_cat10 = 10000.0;
	radar_delay[i].min_retardo_cat19 = 10000.0; radar_delay[i].min_retardo_cat20 = 10000.0;
	radar_delay[i].min_retardo_cat21 = 10000.0;
	radar_delay[i].min_retardo_cat34 = 10000.0; radar_delay[i].min_retardo_cat48 = 10000.0;
	radar_delay[i].segmentos_max_cat1 = 0; radar_delay[i].segmentos_max_cat2 = 0;
	radar_delay[i].segmentos_max_cat8 = 0; radar_delay[i].segmentos_max_cat10 = 0;
	radar_delay[i].segmentos_max_cat19 = 0; radar_delay[i].segmentos_max_cat20 = 0;
	radar_delay[i].segmentos_max_cat21 = 0;
	radar_delay[i].segmentos_max_cat34 = 0; radar_delay[i].segmentos_max_cat48 = 0;
	radar_delay[i].segmentos_ptr_cat1 = 0; radar_delay[i].segmentos_ptr_cat2 = 0;
	radar_delay[i].segmentos_ptr_cat8 = 0; radar_delay[i].segmentos_ptr_cat10 = 0;
	radar_delay[i].segmentos_ptr_cat19 = 0; radar_delay[i].segmentos_ptr_cat20 = 0;
	radar_delay[i].segmentos_ptr_cat21 = 0;
	radar_delay[i].segmentos_ptr_cat34 = 0; radar_delay[i].segmentos_ptr_cat48 = 0;
    }
}

void radar_delay_free(void) {
int i;

    for (i=0; i < MAX_RADAR_NUMBER; i++) {
	mem_free(radar_delay[i].segmentos_cat1);
	mem_free(radar_delay[i].segmentos_cat2);
	mem_free(radar_delay[i].segmentos_cat8);
	mem_free(radar_delay[i].segmentos_cat10);
	mem_free(radar_delay[i].segmentos_cat19);
	mem_free(radar_delay[i].segmentos_cat20);
	mem_free(radar_delay[i].segmentos_cat21);
	mem_free(radar_delay[i].segmentos_cat34);
	mem_free(radar_delay[i].segmentos_cat48);
    }
    mem_free(radar_delay);
}


int main(int argc, char *argv[]) {
    struct datablock_plot dbp;
    int dbplen, i, j;
    socklen_t addrlen;
    struct timeval t2, old_t2;

    startup();
    log_printf(LOG_NORMAL, "client_time_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	    
    server_connect();
    radar_delay_alloc();
    radar_delay_clear();
    if (gettimeofday(&t2, NULL) == -1) {
	log_printf(LOG_ERROR, "gettimeofday %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
    old_t2.tv_sec = t2.tv_sec - (t2.tv_sec % ((int)UPDATE_TIME));

/* // TESTEO DE INSERCION EN LISTA, OK
    {
	struct sorted_list *p = radar_delay[0].sorted_list_cat1;
	int i=0;
	for(i=0;i<100000;i++) {
	    insertList(&p, i, rand());
	}
	i=0;
	
	while(p!=NULL) {
	    i++;
	    p = p->next;
	}
	log_printf(LOG_NORMAL, "cuenta %d\n", i);
    }
    return 0;
*/
    while (!forced_exit) {
        struct timeval timeout;
	float diff = 0.0;
	int select_return;
	dbplen = sizeof(dbp);
	addrlen = sizeof(addr);

	timeout.tv_sec = SERVER_TIMEOUT_SEC;
	timeout.tv_usec = 0;
	select_return = select(s+1, &reader_set, NULL, NULL, &timeout);
	
	if (select_return > 0) {
	    FD_SET(s, &reader_set);
	    if (recvfrom(s, &dbp, dbplen, 0, (struct sockaddr *) &addr, &addrlen) < 0) {
		log_printf(LOG_ERROR, "recvfrom\n");
		exit(EXIT_FAILURE);
	    }
	    if (dbp.cat == CAT_255) {
		log_printf(LOG_ERROR, "fin de fichero\n");
		forced_exit = true;
	    }
//	    log_printf(LOG_NORMAL,"0) %03d %03d\n", dbp.sac, dbp.sic);
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		i=0;
//		log_printf(LOG_NORMAL,"A) %03d %03d (%3.3f)\n", dbp.sac, dbp.sic, diff);
		while ( (i < MAX_RADAR_NUMBER) &&
		    ( (dbp.sac != radar_delay[i].sac) ||
		    (dbp.sic != radar_delay[i].sic) ) &&
		    ( (radar_delay[i].sac != 0) ||
		    (radar_delay[i].sic != 0) ) ) {
		    i++; 
//		    log_printf(LOG_NORMAL,"B) %03d %03d %03d\n", i, radar_delay[i].sac, radar_delay[i].sic);
		}
//		log_printf(LOG_NORMAL,"C) %03d %03d (%3.3f)\n", radar_delay[i].sac, 
//		    radar_delay[i].sic, diff);
//		log_printf(LOG_NORMAL, "D) ---------------(%d)\n", i);

		if (i == MAX_RADAR_NUMBER) {
		    log_printf(LOG_ERROR, "no hay suficientes slots para radares\n");
		    exit(EXIT_FAILURE);
		} else {
		    if ( (!radar_delay[i].sac) &&
			 (!radar_delay[i].sic) ) {
			radar_delay[i].sac = dbp.sac;
			radar_delay[i].sic = dbp.sic;
		    }
		}
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
                    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >= (86400-512)) { // añadido para solucionar un bug v0.63
		    diff -= 86400;
		}

		if (fabs(diff) >= 8.0) {
		    log_printf(LOG_ERROR, "retardo mayor de 8 segundos\n");
		    continue;
		    // exit(EXIT_FAILURE);
		}

		switch (dbp.cat) {
		case CAT_01 : {
				if (dbp.flag_test == 1)
				    continue;
				radar_delay[i].cuenta_plot_cat1++;
				radar_delay[i].suma_retardos_cat1+=diff;
				radar_delay[i].suma_retardos_cuad_cat1+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat1)
				    radar_delay[i].max_retardo_cat1 = diff;
				if (diff < radar_delay[i].min_retardo_cat1)
				    radar_delay[i].min_retardo_cat1 = diff;
				radar_delay[i].segmentos_cat1[(int) floorf((diff+8.0)*10000.0/50.0)]++;
//				log_printf(LOG_NORMAL, "%3.4f;%3.4f\n", diff, ((int) floorf((diff+8.0)*10000.0/50.0))*0.005-8.0 );
				break; }
		case CAT_02 : { 
				radar_delay[i].cuenta_plot_cat2++;
			        radar_delay[i].suma_retardos_cat2+=diff;
				radar_delay[i].suma_retardos_cuad_cat2+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat2)
				    radar_delay[i].max_retardo_cat2 = diff;
				if (diff < radar_delay[i].min_retardo_cat2)
				    radar_delay[i].min_retardo_cat2 = diff;
				radar_delay[i].segmentos_cat2[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		case CAT_08 : { 
				radar_delay[i].cuenta_plot_cat8++;
			        radar_delay[i].suma_retardos_cat8+=diff;
				radar_delay[i].suma_retardos_cuad_cat8+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat8)
				    radar_delay[i].max_retardo_cat8 = diff;
				if (diff < radar_delay[i].min_retardo_cat8)
				    radar_delay[i].min_retardo_cat8 = diff;
				radar_delay[i].segmentos_cat8[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		case CAT_10 : { 
				radar_delay[i].cuenta_plot_cat10++;
			        radar_delay[i].suma_retardos_cat10+=diff;
				radar_delay[i].suma_retardos_cuad_cat10+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat10)
				    radar_delay[i].max_retardo_cat10 = diff;
				if (diff < radar_delay[i].min_retardo_cat10)
				    radar_delay[i].min_retardo_cat10 = diff;
				radar_delay[i].segmentos_cat10[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		case CAT_19 : { 
				radar_delay[i].cuenta_plot_cat19++;
			        radar_delay[i].suma_retardos_cat19+=diff;
				radar_delay[i].suma_retardos_cuad_cat19+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat19)
				    radar_delay[i].max_retardo_cat19 = diff;
				if (diff < radar_delay[i].min_retardo_cat19)
				    radar_delay[i].min_retardo_cat19 = diff;
				radar_delay[i].segmentos_cat19[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		case CAT_20 : { 
				radar_delay[i].cuenta_plot_cat20++;
			        radar_delay[i].suma_retardos_cat20+=diff;
				radar_delay[i].suma_retardos_cuad_cat20+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat20)
				    radar_delay[i].max_retardo_cat20 = diff;
				if (diff < radar_delay[i].min_retardo_cat20)
				    radar_delay[i].min_retardo_cat20 = diff;
				radar_delay[i].segmentos_cat20[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		case CAT_21 : { 
				radar_delay[i].cuenta_plot_cat21++;
			        radar_delay[i].suma_retardos_cat21+=diff;
				radar_delay[i].suma_retardos_cuad_cat21+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat21)
				    radar_delay[i].max_retardo_cat21 = diff;
				if (diff < radar_delay[i].min_retardo_cat21)
				    radar_delay[i].min_retardo_cat21 = diff;
				radar_delay[i].segmentos_cat21[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		case CAT_34 : { 
				radar_delay[i].cuenta_plot_cat34++;
			        radar_delay[i].suma_retardos_cat34+=diff;
				radar_delay[i].suma_retardos_cuad_cat34+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat34)
				    radar_delay[i].max_retardo_cat34 = diff;
				if (diff < radar_delay[i].min_retardo_cat34)
				    radar_delay[i].min_retardo_cat34 = diff;
				radar_delay[i].segmentos_cat34[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		case CAT_48 : { 
				radar_delay[i].cuenta_plot_cat48++;
			        radar_delay[i].suma_retardos_cat48+=diff;
				radar_delay[i].suma_retardos_cuad_cat48+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat48)
				    radar_delay[i].max_retardo_cat48 = diff;
				if (diff < radar_delay[i].min_retardo_cat48)
				    radar_delay[i].min_retardo_cat48 = diff;
				radar_delay[i].segmentos_cat48[(int) floorf((diff+8.0)*10000.0/50.0)]++;
				break; }
		default : {
				log_printf(LOG_ERROR, "categoria desconocida %08X", dbp.cat);
				exit(EXIT_FAILURE);
				break; }
		}
	    }
        } else if (select_return == 0) {
	    log_printf(LOG_ERROR, "sin conexion\n");
	    close(s);
	    server_connect();
	} else {
	    log_printf(LOG_ERROR, "socket error: %s\n", strerror(errno));
	    close(s);
	    server_connect();
	}
        
	if (gettimeofday(&t2, NULL) == -1) {
	    log_printf(LOG_ERROR, "gettimeofday %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	// t2 is current_date
	
	t2.tv_sec -= (t2.tv_sec % ((int)UPDATE_TIME));

//	if ( ( ((float)t2.tv_sec + t2.tv_usec/1000000.0) -
//	    ((float)t.tv_sec + t.tv_usec/1000000.0) > UPDATE_TIME ) ||
//	    forced_exit) {

	if ( (t2.tv_sec >= (old_t2.tv_sec + UPDATE_TIME)) || forced_exit ) {
	    struct timeval calcdelay1; struct timeval calcdelay2; float calcdelay = 0.0;
	    char *sac_s=0, *sic_l=0;
	    float l1=0.0, l2=0.0, l8=0.0, l10=0.0;
            float l19=0.0, l20=0.0;
            float l21=0.0, l34=0.0, l48=0.0;
	    float sc1_1=0.0, sc1_2=0.0, sc1_3=0.0;
	    float sc2_1=0.0, sc2_2=0.0, sc2_3=0.0;
	    float sc8_1=0.0, sc8_2=0.0, sc8_3=0.0;
	    float sc10_1=0.0, sc10_2=0.0, sc10_3=0.0;
	    float sc19_1=0.0, sc19_2=0.0, sc19_3=0.0;
	    float sc20_1=0.0, sc20_2=0.0, sc20_3=0.0;
	    float sc21_1=0.0, sc21_2=0.0, sc21_3=0.0;
	    float sc34_1=0.0, sc34_2=0.0, sc34_3=0.0;
	    float sc48_1=0.0, sc48_2=0.0, sc48_3=0.0;
	    float moda=0.0, p99_cat1=0.0, p99_cat2=0.0;
	    float p99_cat8=0.0, p99_cat10=0.0, p99_cat19=0.0;
	    float p99_cat20=0.0, p99_cat21=0.0;
	    float p99_cat34=0.0, p99_cat48=0.0;
//	    struct current_time_tm tm;
	    
	    old_t2.tv_sec = t2.tv_sec;

	    gettimeofday(&calcdelay1, NULL);
	    log_printf(LOG_NORMAL, "%-24s\tCAT\tplots\tmedia\tdesv\tmoda\tmax\tmin\tp99\n", "RADAR");
	    for(i=0; i<MAX_RADAR_NUMBER; i++) {
		if (radar_delay[i].sac || radar_delay[i].sic) {
		    sac_s = ast_get_SACSIC((unsigned char *) &radar_delay[i].sac,
			(unsigned char *) &radar_delay[i].sic, GET_SAC_SHORT);
		    sic_l = ast_get_SACSIC((unsigned char *) &radar_delay[i].sac, 
			(unsigned char *) &radar_delay[i].sic, GET_SIC_LONG);

		    for(j=0; j<MAX_SEGMENT_NUMBER; j++) {
			if (radar_delay[i].cuenta_plot_cat1>0) {
			    if (radar_delay[i].segmentos_cat1[j] > radar_delay[i].segmentos_max_cat1) {
				radar_delay[i].segmentos_ptr_cat1 = j;
				radar_delay[i].segmentos_max_cat1 = radar_delay[i].segmentos_cat1[j];
			    }
			    if (radar_delay[i].segmentos_cat1[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat1, j, radar_delay[i].segmentos_cat1[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat2>0) {
			    if (radar_delay[i].segmentos_cat2[j] > radar_delay[i].segmentos_max_cat2) {
				radar_delay[i].segmentos_ptr_cat2 = j;
				radar_delay[i].segmentos_max_cat2 = radar_delay[i].segmentos_cat2[j];
			    }
			    if (radar_delay[i].segmentos_cat2[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat2, j, radar_delay[i].segmentos_cat2[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat8>0) {
			    if (radar_delay[i].segmentos_cat8[j] > radar_delay[i].segmentos_max_cat8) {
				radar_delay[i].segmentos_ptr_cat8 = j;
				radar_delay[i].segmentos_max_cat8 = radar_delay[i].segmentos_cat8[j];
			    }
			    if (radar_delay[i].segmentos_cat8[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat8, j, radar_delay[i].segmentos_cat8[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat10>0) {
			    if (radar_delay[i].segmentos_cat10[j] > radar_delay[i].segmentos_max_cat10) {
				radar_delay[i].segmentos_ptr_cat10 = j;
				radar_delay[i].segmentos_max_cat10 = radar_delay[i].segmentos_cat10[j];
			    }
			    if (radar_delay[i].segmentos_cat10[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat10, j, radar_delay[i].segmentos_cat10[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat19>0) {
			    if (radar_delay[i].segmentos_cat19[j] > radar_delay[i].segmentos_max_cat19) {
				radar_delay[i].segmentos_ptr_cat19 = j;
				radar_delay[i].segmentos_max_cat19 = radar_delay[i].segmentos_cat19[j];
			    }
			    if (radar_delay[i].segmentos_cat19[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat19, j, radar_delay[i].segmentos_cat19[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat20>0) {
			    if (radar_delay[i].segmentos_cat20[j] > radar_delay[i].segmentos_max_cat20) {
				radar_delay[i].segmentos_ptr_cat20 = j;
				radar_delay[i].segmentos_max_cat20 = radar_delay[i].segmentos_cat20[j];
			    }
			    if (radar_delay[i].segmentos_cat20[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat20, j, radar_delay[i].segmentos_cat20[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat21>0) {
			    if (radar_delay[i].segmentos_cat21[j] > radar_delay[i].segmentos_max_cat21) {
				radar_delay[i].segmentos_ptr_cat21 = j;
				radar_delay[i].segmentos_max_cat21 = radar_delay[i].segmentos_cat21[j];
			    }
			    if (radar_delay[i].segmentos_cat21[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat21, j, radar_delay[i].segmentos_cat21[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat34>0) {
			    if (radar_delay[i].segmentos_cat34[j] > radar_delay[i].segmentos_max_cat34) {
				radar_delay[i].segmentos_ptr_cat34 = j;
				radar_delay[i].segmentos_max_cat34 = radar_delay[i].segmentos_cat34[j];
			    }
			    if (radar_delay[i].segmentos_cat34[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat34, j, radar_delay[i].segmentos_cat34[j]);
			    }
			}
			if (radar_delay[i].cuenta_plot_cat48>0) {
			    if (radar_delay[i].segmentos_cat48[j] > radar_delay[i].segmentos_max_cat48) {
				radar_delay[i].segmentos_ptr_cat48 = j;
				radar_delay[i].segmentos_max_cat48 = radar_delay[i].segmentos_cat48[j];
			    }
			    if (radar_delay[i].segmentos_cat48[j]>0) {
				insertList(&radar_delay[i].sorted_list_cat48, j, radar_delay[i].segmentos_cat48[j]);
			    }
			}
		    }
		    if (radar_delay[i].cuenta_plot_cat1>0) {
			l1 =    ( ((float) radar_delay[i].segmentos_ptr_cat1) / 10000.0*50.0 ) - 8.0;
			sc1_1 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1]) / 10000.0*50.0 ) - 8.0;
			sc1_2 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1 + 1]) / 10000.0*50.0 ) - 8.0;
			sc1_3 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat1!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat1;
			    struct sorted_list *p_old = NULL;
			    long count = 0; 
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat1)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>=count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat1 = p->segment; }
			}
			moda = l1 + ( (sc1_1 - sc1_3) / ( (sc1_1 - sc1_3) + (sc1_1 - sc1_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t 1\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat1,
			    radar_delay[i].suma_retardos_cat1/radar_delay[i].cuenta_plot_cat1,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat1 / radar_delay[i].cuenta_plot_cat1) - pow(radar_delay[i].suma_retardos_cat1 / radar_delay[i].cuenta_plot_cat1,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
			    radar_delay[i].max_retardo_cat1 == -10000 ? 0 : radar_delay[i].max_retardo_cat1,
			    radar_delay[i].min_retardo_cat1 ==  10000 ? 0 : radar_delay[i].min_retardo_cat1,
			    p99_cat1);
		    }
		    if (radar_delay[i].cuenta_plot_cat2>0) {
			l2 =    ( ((float) radar_delay[i].segmentos_ptr_cat2) / 10000.0*50.0 ) - 8.0;
			sc2_1 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2]) / 10000.0*50.0 ) - 8.0;
			sc2_2 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2 + 1]) / 10000.0*50.0 ) - 8.0;
			sc2_3 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat2!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat2;
			    struct sorted_list *p_old = NULL;
			    long count = 0; 
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat2)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat2 = p->segment; }
			}
			moda = l2 + ( (sc2_1 - sc2_3) / ( (sc2_1 - sc2_3) + (sc2_1 - sc2_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t 2\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat2,
			    radar_delay[i].suma_retardos_cat2/radar_delay[i].cuenta_plot_cat2,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat2 / radar_delay[i].cuenta_plot_cat2) - pow(radar_delay[i].suma_retardos_cat2 / radar_delay[i].cuenta_plot_cat2,2)),
			    (moda < -7.993) || (moda > 7.996) ? 0 : moda,
			    radar_delay[i].max_retardo_cat2 == -10000 ? 0 : radar_delay[i].max_retardo_cat2,
			    radar_delay[i].min_retardo_cat2 ==  10000 ? 0 : radar_delay[i].min_retardo_cat2,
			    p99_cat2);
		    }
		    if (radar_delay[i].cuenta_plot_cat8>0) {
			l8 =    ( ((float) radar_delay[i].segmentos_ptr_cat8) / 10000.0*50.0 ) - 8.0;
			sc8_1 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8]) / 10000.0*50.0 ) - 8.0;
			sc8_2 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8 + 1]) / 10000.0*50.0 ) - 8.0;
			sc8_3 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat8!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat8;
			    struct sorted_list *p_old = NULL;
			    long count = 0; 
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat8)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat8 = p->segment; }
			}
			moda = l8 + ( (sc8_1 - sc8_3) / ( (sc8_1 - sc8_3) + (sc8_1 - sc8_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t 8\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat8,
			    radar_delay[i].suma_retardos_cat8/radar_delay[i].cuenta_plot_cat8,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat8 / radar_delay[i].cuenta_plot_cat8) - pow(radar_delay[i].suma_retardos_cat8 / radar_delay[i].cuenta_plot_cat8,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
			    radar_delay[i].max_retardo_cat8 == -10000 ? 0 : radar_delay[i].max_retardo_cat8,
			    radar_delay[i].min_retardo_cat8 ==  10000 ? 0 : radar_delay[i].min_retardo_cat8,
			    p99_cat8);
		    }
		    if (radar_delay[i].cuenta_plot_cat10>0) {
			l10 =    ( ((float) radar_delay[i].segmentos_ptr_cat10) / 10000.0*50.0 ) - 8.0;
			sc10_1 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10]) / 10000.0*50.0 ) - 8.0;
			sc10_2 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10 + 1]) / 10000.0*50.0 ) - 8.0;
			sc10_3 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat10!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat10;
			    struct sorted_list *p_old = NULL;
			    long count = 0; 
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat10)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat10 = p->segment; }
			}
			moda = l10 + ( (sc10_1 - sc10_3) / ( (sc10_1 - sc10_3) + (sc10_1 - sc10_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t10\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat10,
			    radar_delay[i].suma_retardos_cat10/radar_delay[i].cuenta_plot_cat10,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat10 / radar_delay[i].cuenta_plot_cat10) - pow(radar_delay[i].suma_retardos_cat10 / radar_delay[i].cuenta_plot_cat10,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
		    	    radar_delay[i].max_retardo_cat10 == -10000 ? 0 : radar_delay[i].max_retardo_cat10,
			    radar_delay[i].min_retardo_cat10 ==  10000 ? 0 : radar_delay[i].min_retardo_cat10,
			    p99_cat10);
		    }
		    if (radar_delay[i].cuenta_plot_cat19>0) {
			l19 =    ( ((float) radar_delay[i].segmentos_ptr_cat19) / 10000.0*50.0 ) - 8.0;
			sc19_1 = ( ((float) radar_delay[i].segmentos_cat19[radar_delay[i].segmentos_ptr_cat19]) / 10000.0*50.0 ) - 8.0;
			sc19_2 = ( ((float) radar_delay[i].segmentos_cat19[radar_delay[i].segmentos_ptr_cat19 + 1]) / 10000.0*50.0 ) - 8.0;
			sc19_3 = ( ((float) radar_delay[i].segmentos_cat19[radar_delay[i].segmentos_ptr_cat19 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat19!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat19;
			    struct sorted_list *p_old = NULL;
			    long count = 0;
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat19)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat19 = p->segment; }
			}
			moda = l19 + ( (sc19_1 - sc19_3) / ( (sc19_1 - sc19_3) + (sc19_1 - sc19_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t19\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat19,
			    radar_delay[i].suma_retardos_cat19/radar_delay[i].cuenta_plot_cat19,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat19 / radar_delay[i].cuenta_plot_cat19) - pow(radar_delay[i].suma_retardos_cat19 / radar_delay[i].cuenta_plot_cat19,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
                            radar_delay[i].max_retardo_cat19 == -10000 ? 0 : radar_delay[i].max_retardo_cat19,
                            radar_delay[i].min_retardo_cat19 ==  10000 ? 0 : radar_delay[i].min_retardo_cat19,
			    p99_cat19);
		    }
		    if (radar_delay[i].cuenta_plot_cat20>0) {
			l20 =    ( ((float) radar_delay[i].segmentos_ptr_cat20) / 10000.0*50.0 ) - 8.0;
			sc20_1 = ( ((float) radar_delay[i].segmentos_cat20[radar_delay[i].segmentos_ptr_cat20]) / 10000.0*50.0 ) - 8.0;
			sc20_2 = ( ((float) radar_delay[i].segmentos_cat20[radar_delay[i].segmentos_ptr_cat20 + 1]) / 10000.0*50.0 ) - 8.0;
			sc20_3 = ( ((float) radar_delay[i].segmentos_cat20[radar_delay[i].segmentos_ptr_cat20 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat20!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat20;
			    struct sorted_list *p_old = NULL;
			    long count = 0;
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat20)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat20 = p->segment; }
			}
			moda = l20 + ( (sc20_1 - sc20_3) / ( (sc20_1 - sc20_3) + (sc20_1 - sc20_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t20\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat20,
			    radar_delay[i].suma_retardos_cat20/radar_delay[i].cuenta_plot_cat20,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat20 / radar_delay[i].cuenta_plot_cat20) - pow(radar_delay[i].suma_retardos_cat20 / radar_delay[i].cuenta_plot_cat20,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
                            radar_delay[i].max_retardo_cat20 == -10000 ? 0 : radar_delay[i].max_retardo_cat20,
                            radar_delay[i].min_retardo_cat20 ==  10000 ? 0 : radar_delay[i].min_retardo_cat20,
			    p99_cat20);
		    }
		    if (radar_delay[i].cuenta_plot_cat21>0) {
			l21 =    ( ((float) radar_delay[i].segmentos_ptr_cat21) / 10000.0*50.0 ) - 8.0;
			sc21_1 = ( ((float) radar_delay[i].segmentos_cat21[radar_delay[i].segmentos_ptr_cat21]) / 10000.0*50.0 ) - 8.0;
			sc21_2 = ( ((float) radar_delay[i].segmentos_cat21[radar_delay[i].segmentos_ptr_cat21 + 1]) / 10000.0*50.0 ) - 8.0;
			sc21_3 = ( ((float) radar_delay[i].segmentos_cat21[radar_delay[i].segmentos_ptr_cat21 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat21!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat21;
			    struct sorted_list *p_old = NULL;
			    long count = 0;
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat21)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat21 = p->segment; }
			}
			moda = l21 + ( (sc21_1 - sc21_3) / ( (sc21_1 - sc21_3) + (sc21_1 - sc21_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t21\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat21,
			    radar_delay[i].suma_retardos_cat21/radar_delay[i].cuenta_plot_cat21,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat21 / radar_delay[i].cuenta_plot_cat21) - pow(radar_delay[i].suma_retardos_cat21 / radar_delay[i].cuenta_plot_cat21,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
			    radar_delay[i].max_retardo_cat21 == -10000 ? 0 : radar_delay[i].max_retardo_cat21,
			    radar_delay[i].min_retardo_cat21 ==  10000 ? 0 : radar_delay[i].min_retardo_cat21,
			    p99_cat21);
		    }
		    if (radar_delay[i].cuenta_plot_cat34>0) {
			l34 =    ( ((float) radar_delay[i].segmentos_ptr_cat34) / 10000.0*50.0 ) - 8.0;
			sc34_1 = ( ((float) radar_delay[i].segmentos_cat34[radar_delay[i].segmentos_ptr_cat34]) / 10000.0*50.0 ) - 8.0;
			sc34_2 = ( ((float) radar_delay[i].segmentos_cat34[radar_delay[i].segmentos_ptr_cat34 + 1]) / 10000.0*50.0 ) - 8.0;
			sc34_3 = ( ((float) radar_delay[i].segmentos_cat34[radar_delay[i].segmentos_ptr_cat34 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat34!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat34;
			    struct sorted_list *p_old = NULL;
			    long count = 0;
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat34)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat34 = p->segment; }
			}
			moda = l34 + ( (sc34_1 - sc34_3) / ( (sc34_1 - sc34_3) + (sc34_1 - sc34_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t34\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat34,
			    radar_delay[i].suma_retardos_cat34/radar_delay[i].cuenta_plot_cat34,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat34 / radar_delay[i].cuenta_plot_cat34) - pow(radar_delay[i].suma_retardos_cat34 / radar_delay[i].cuenta_plot_cat34,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
			    radar_delay[i].max_retardo_cat34 == -10000 ? 0 : radar_delay[i].max_retardo_cat34,
			    radar_delay[i].min_retardo_cat34 ==  10000 ? 0 : radar_delay[i].min_retardo_cat34,
			    p99_cat34);
		    }
		    if (radar_delay[i].cuenta_plot_cat48>0) {
			l48 =    ( ((float) radar_delay[i].segmentos_ptr_cat48) / 10000.0*50.0 ) - 8.0;
			sc48_1 = ( ((float) radar_delay[i].segmentos_cat48[radar_delay[i].segmentos_ptr_cat48]) / 10000.0*50.0 ) - 8.0;
			sc48_2 = ( ((float) radar_delay[i].segmentos_cat48[radar_delay[i].segmentos_ptr_cat48 + 1]) / 10000.0*50.0 ) - 8.0;
			sc48_3 = ( ((float) radar_delay[i].segmentos_cat48[radar_delay[i].segmentos_ptr_cat48 - 1]) / 10000.0*50.0 ) - 8.0;
			if(radar_delay[i].sorted_list_cat48!=NULL) {
			    struct sorted_list *p = radar_delay[i].sorted_list_cat48;
			    struct sorted_list *p_old = NULL;
			    long count = 0;
			    float count_percentil_99 = ((float)radar_delay[i].cuenta_plot_cat48)*99.0/100.0;
			    while(p!=NULL) {
				if ((count + p->count)>count_percentil_99) { break; }
				count += p->count; p_old = p; p = p->next;
			    }
			    if (p!=NULL && p_old!=NULL) { p99_cat48 = p->segment; }
			}
			moda = l48 + ( (sc48_1 - sc48_3) / ( (sc48_1 - sc48_3) + (sc48_1 - sc48_2) ) ) * 0.005;
			log_printf(LOG_NORMAL, "%-3s/%-20s\t48\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat48,
			    radar_delay[i].suma_retardos_cat48/radar_delay[i].cuenta_plot_cat48,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat48 / radar_delay[i].cuenta_plot_cat48) - pow(radar_delay[i].suma_retardos_cat48 / radar_delay[i].cuenta_plot_cat48,2)),
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
			    radar_delay[i].max_retardo_cat48 == -10000 ? 0 : radar_delay[i].max_retardo_cat48,
			    radar_delay[i].min_retardo_cat48 ==  10000 ? 0 : radar_delay[i].min_retardo_cat48,
			    p99_cat48);
		    }
		    log_printf(LOG_NORMAL, "---------------------------------------------------------------------------------------------\n");
		    mem_free(sac_s);
		    mem_free(sic_l);
		}
	    }	
//	    t.tv_sec = t2.tv_sec; t.tv_usec = t2.tv_usec;
	    radar_delay_clear();
	    gettimeofday(&calcdelay2, NULL);
	    calcdelay = ((float)calcdelay2.tv_sec + calcdelay2.tv_usec/1000000.0)-((float)calcdelay1.tv_sec + calcdelay1.tv_usec/1000000.0);
	    log_printf(LOG_NORMAL, "==%03.4f==%s", calcdelay, asctime(gmtime(&t2.tv_sec)));
	    
	}
//	log_flush();
    }

    log_printf(LOG_NORMAL, "end...\n");
//    log_flush();

    radar_delay_free();

    exit(EXIT_SUCCESS);
}

