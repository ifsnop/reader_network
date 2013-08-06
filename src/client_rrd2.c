// para la implementacion de un client_rrd con percentiles99
#include "includes.h"

#define SERVER_TIMEOUT_SEC 60

struct radar_delay_s {
    int first_time;
    unsigned char sac,sic;
    long cuenta_plot_cat1, cuenta_plot_cat2;
    long cuenta_plot_cat8, cuenta_plot_cat10;
    float suma_retardos_cat1, suma_retardos_cat2;
    float suma_retardos_cat8, suma_retardos_cat10;
    float suma_retardos_cuad_cat1, suma_retardos_cuad_cat2;
    float suma_retardos_cuad_cat8, suma_retardos_cuad_cat10;
    float max_retardo_cat1, max_retardo_cat2;
    float max_retardo_cat8, max_retardo_cat10;
    float min_retardo_cat1, min_retardo_cat2;
    float min_retardo_cat8, min_retardo_cat10;
    int *segmentos_cat1, *segmentos_cat2;
    int *segmentos_cat8, *segmentos_cat10;
    int segmentos_max_cat1, segmentos_max_cat2;
    int segmentos_max_cat8, segmentos_max_cat10;
    int segmentos_ptr_cat1, segmentos_ptr_cat2;
    int segmentos_ptr_cat8, segmentos_ptr_cat10;
};

struct radar_delay_s *radar_delay;

struct ip_mreq mreq;
struct sockaddr_in addr;
fd_set reader_set;
int s, yes = 1;
bool forced_exit = false;
time_t t3;

void create_database(const char *sac, const char *sic) {
char *tmp;

    tmp = mem_alloc(512);
    sprintf(tmp, "./rrd_create.sh %s_%s_CAT1 %ld", sac, sic, t3);
    system(tmp);
    sprintf(tmp, "./rrd_create.sh %s_%s_CAT2 %ld", sac, sic, t3);    
    system(tmp);
    mem_free(tmp);
    return;
}

void setup_time(void) {
struct timeval t;
struct tm *t2;

    gettimeofday(&t, NULL);
    t2 = gmtime(&t.tv_sec);
    t2->tm_sec = 0; t2->tm_min = 0; t2->tm_hour = 0;
    t3 = mktime(t2); //segundos a las 00:00:00 de hoy
    return;
}
		    
void server_connect(void) {

    FD_ZERO(&reader_set);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PLOTS_PORT);
    if ( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	fprintf(stderr, "socket %s\n", strerror(errno));
	exit(1);
    }
    FD_SET(s, &reader_set);

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if ( bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	fprintf(stderr, "bind %s\n", strerror(errno));
	exit(1);
    }
				
    mreq.imr_interface.s_addr = inet_addr("127.0.0.1"); //nHostAddress;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_PLOTS_GROUP);
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
	fprintf(stderr, "setsocktperror\n");
	exit(1);
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
	radar_delay[i].first_time = 0;
    }
    return;
}

void radar_delay_clear(void) {
int i;
    for (i=0; i < MAX_RADAR_NUMBER; i++) {
	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
    	radar_delay[i].sac = '\0'; radar_delay[i].sic = '\0';
	radar_delay[i].cuenta_plot_cat1 = 0; radar_delay[i].cuenta_plot_cat2 = 0;
	radar_delay[i].cuenta_plot_cat8 = 0; radar_delay[i].cuenta_plot_cat10 = 0;
	radar_delay[i].suma_retardos_cat1 = 0; radar_delay[i].suma_retardos_cat2 = 0;
	radar_delay[i].suma_retardos_cat8 = 0; radar_delay[i].suma_retardos_cat10 = 0;
	radar_delay[i].suma_retardos_cuad_cat1 = 0; radar_delay[i].suma_retardos_cuad_cat2 = 0;
	radar_delay[i].suma_retardos_cuad_cat8 = 0; radar_delay[i].suma_retardos_cuad_cat10 = 0;
	radar_delay[i].max_retardo_cat1 = -10000.0; radar_delay[i].max_retardo_cat2 = -10000.0;
	radar_delay[i].max_retardo_cat8 = -10000.0; radar_delay[i].max_retardo_cat10 = -10000.0;
	radar_delay[i].min_retardo_cat1 = 10000.0; radar_delay[i].min_retardo_cat2 = 10000.0;
        radar_delay[i].min_retardo_cat8 = 10000.0; radar_delay[i].min_retardo_cat10 = 10000.0;
	radar_delay[i].segmentos_max_cat1 = 0; radar_delay[i].segmentos_max_cat2 = 0;
	radar_delay[i].segmentos_max_cat8 = 0; radar_delay[i].segmentos_max_cat10 = 0;
	radar_delay[i].segmentos_ptr_cat1 = 0; radar_delay[i].segmentos_ptr_cat2 = 0;
	radar_delay[i].segmentos_ptr_cat8 = 0; radar_delay[i].segmentos_ptr_cat10 = 0;
    }
}

void radar_delay_free(void) {
int i;

    for (i=0; i < MAX_RADAR_NUMBER; i++) {
	mem_free(radar_delay[i].segmentos_cat1);
	mem_free(radar_delay[i].segmentos_cat2);
	mem_free(radar_delay[i].segmentos_cat8);
	mem_free(radar_delay[i].segmentos_cat10);
    }
    mem_free(radar_delay);
}

void radar_delay_iterate(int i) {
int j;

    for(j=0; j<3200; j++) {
        if (radar_delay[i].segmentos_cat1[j] > radar_delay[i].segmentos_max_cat1) {
            radar_delay[i].segmentos_ptr_cat1 = j;
    	    radar_delay[i].segmentos_max_cat1 = radar_delay[i].segmentos_cat1[j];
	}
	if (radar_delay[i].segmentos_cat2[j] > radar_delay[i].segmentos_max_cat2) {
	    radar_delay[i].segmentos_ptr_cat2 = j;
	    radar_delay[i].segmentos_max_cat2 = radar_delay[i].segmentos_cat2[j];
	}
	if (radar_delay[i].segmentos_cat8[j] > radar_delay[i].segmentos_max_cat8) {
	    radar_delay[i].segmentos_ptr_cat8 = j;
	    radar_delay[i].segmentos_max_cat8 = radar_delay[i].segmentos_cat8[j];
	}
	if (radar_delay[i].segmentos_cat10[j] > radar_delay[i].segmentos_max_cat10) {
	    radar_delay[i].segmentos_ptr_cat10 = j;
	    radar_delay[i].segmentos_max_cat10 = radar_delay[i].segmentos_cat10[j];
	}
    }
    return;
}

void fail (const char *fmt, ...)
{
    va_list ap;
    va_start (ap,fmt);
    if (fmt != NULL) {
        vfprintf (stderr,fmt,ap);
        fflush (stderr);
    }
    va_end (ap);
    exit (EXIT_FAILURE);
}
					    

int main(int argc, char *argv[]) {
    struct datablock_plot dbp;
    int dbplen, i;
    socklen_t addrlen;
    long time_old = -1;


    mem_open(fail);
//    atexit(mem_close);
						
    fprintf(stderr,"init...\n");
    
    setup_time();
    server_connect();
    radar_delay_alloc();
    radar_delay_clear();
    
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
		fprintf(stderr, "recvfrom\n");
		exit(EXIT_FAILURE);
	    }    
	    if (dbp.cat == CAT_255) {
		fprintf(stderr, "fin de fichero\n");
		forced_exit = true;
	    } 

	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		i=0;
		while ( (i < MAX_RADAR_NUMBER) && 
		    ( (dbp.sac != radar_delay[i].sac) ||
		    (dbp.sic != radar_delay[i].sic) ) &&
		    (radar_delay[i].sac != 0) &&
		    (radar_delay[i].sic != 0) ) { 
		    i++; 
		}
		if (i == MAX_RADAR_NUMBER) {
		    fprintf(stderr, "no hay suficientes slots para radares\n");
		    exit(EXIT_FAILURE);
		} else {
		    if ( (!radar_delay[i].sac) &&
			(!radar_delay[i].sic) ) {
			radar_delay[i].sac = dbp.sac;
			radar_delay[i].sic = dbp.sic;
		    }
		}
		//printf("pc(%2.4f) plot(%2.4f) diff(%2.4f)\n", dbp.tod_stamp, dbp.tod, diff);
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;   // le sumamos un dia entero para cuadrar el calculo
		}
		if (fabs(diff) >= 16.0) {
		    fprintf(stderr, "retardo mayor de 8 segundos\n");
		    continue;
//		    exit(EXIT_FAILURE);
		}				 

		switch (dbp.cat) {
		case CAT_01 : { 
				radar_delay[i].cuenta_plot_cat1++;
			        radar_delay[i].suma_retardos_cat1+=diff;
				radar_delay[i].suma_retardos_cuad_cat1+=pow(diff,2);
				if (diff > radar_delay[i].max_retardo_cat1)
				    radar_delay[i].max_retardo_cat1 = diff;
				if (diff < radar_delay[i].min_retardo_cat1)
				    radar_delay[i].min_retardo_cat1 = diff;
				radar_delay[i].segmentos_cat1[(int) floorf((diff+8.0)*10000.0/50.0)]++;
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
//		case CAT_10 : { 
//				radar_delay[i].cuenta_plot_cat10++;
//			        radar_delay[i].suma_retardos_cat10+=diff;
//				radar_delay[i].suma_retardos_cuad_cat10+=pow(diff,2);
//				if (diff > radar_delay[i].max_retardo_cat10)
//				    radar_delay[i].max_retardo_cat10 = diff;
//				if (diff < radar_delay[i].min_retardo_cat10)
//				    radar_delay[i].min_retardo_cat10 = diff;
//				radar_delay[i].segmentos_cat10[(int) floorf((diff+8.0)*10000.0/50.0)]++;
//				break; }
		default : {
				fprintf(stderr, "categoria desconocida %08X", dbp.cat);
				exit(EXIT_FAILURE);
				break; }
		}
	    }
        } else if (select_return == 0) {
	    fprintf(stderr, "sin conexion\n");
	    close(s);
	    server_connect();
	} else {
	    fprintf(stderr, "socket error: %s\n", strerror(errno));
	    close(s);
	    server_connect();
	}

	{
	    int i;
	    for (i=0; i < MAX_SEGMENT_NUMBER; i++) {
	        printf("%d:%3.3f-%3.3f:%d\n", i, (i/10000.0*50.0) - 8.0, ((i+1)/10000.0*50.0) - 8.0, radar_delay[0].segmentos_cat1[i]); 
	    }
	}
        
	if (time_old == -1) {
	    div_t d;
	    d = div( ((long)floorf(dbp.tod_stamp)), UPDATE_TIME_RRD);
	    time_old = ((long)floorf(dbp.tod_stamp)) - d.rem;
	} else if ( (time_old + UPDATE_TIME_RRD) < dbp.tod_stamp ) { 

	    char *sac_s=0, *sic_s=0;
	    float l1=0.0, l2=0.0, l8=0.0, l10=0.0;
	    float sc1_1=0.0, sc1_2=0.0, sc1_3=0.0;
    	    float sc2_1=0.0, sc2_2=0.0, sc2_3=0.0;
	    float sc8_1=0.0, sc8_2=0.0, sc8_3=0.0;
	    float sc10_1=0.0, sc10_2=0.0, sc10_3=0.0;
	    float moda;
	    pid_t pid;
	    int status;
	    div_t d;

	    d = div( ((long)floorf(dbp.tod_stamp)), UPDATE_TIME_RRD);
	    time_old = ((long)floorf(dbp.tod_stamp)) - d.rem;
	    for(i=0; i<MAX_RADAR_NUMBER; i++) {
		if (radar_delay[i].sac && radar_delay[i].sic) {
	    	    radar_delay_iterate(i);

		    l1 =    ( ((float) radar_delay[i].segmentos_ptr_cat1) / 10000.0*50.0 ) - 8.0;
		    sc1_1 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1]) / 10000.0*50.0 ) - 8.0;
		    sc1_2 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1 + 1]) / 10000.0*50.0 ) - 8.0;
		    sc1_3 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1 - 1]) / 10000.0*50.0 ) - 8.0;
	    	    l2 =    ( ((float) radar_delay[i].segmentos_ptr_cat2) / 10000.0*50.0 ) - 8.0;
	    	    sc2_1 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2]) / 10000.0*50.0 ) - 8.0;
	    	    sc2_2 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2 + 1]) / 10000.0*50.0 ) - 8.0;
	    	    sc2_3 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2 - 1]) / 10000.0*50.0 ) - 8.0;
	    	    l8 =    ( ((float) radar_delay[i].segmentos_ptr_cat8) / 10000.0*50.0 ) - 8.0;
	    	    sc8_1 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8]) / 10000.0*50.0 ) - 8.0;
	    	    sc8_2 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8 + 1]) / 10000.0*50.0 ) - 8.0;
	    	    sc8_3 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8 - 1]) / 10000.0*50.0 ) - 8.0;
//	    	    l10 =    ( ((float) radar_delay[i].segmentos_ptr_cat10) / 10000.0*50.0 ) - 8.0;
//	    	    sc10_1 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10]) / 10000.0*50.0 ) - 8.0;
//	    	    sc10_2 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10 + 1]) / 10000.0*50.0 ) - 8.0;
//	    	    sc10_3 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10 - 1]) / 10000.0*50.0 ) - 8.0;
		    
        	    sac_s = ast_get_SACSIC((unsigned char *) &radar_delay[i].sac,
		        (unsigned char *) &radar_delay[i].sac, GET_SAC_SHORT);
		    sic_s = ast_get_SACSIC((unsigned char *) &radar_delay[i].sac, 
		        (unsigned char *) &radar_delay[i].sic, GET_SIC_SHORT);

		    if (radar_delay[i].first_time == 0) {
		        radar_delay[i].first_time = 1;
		        create_database(sac_s, sic_s);
		    }
	
    		    pid = fork();
	    
		    if (pid) {
			wait(&status);
		    } else { // calculo e insercion!
			char *tmp;
			
			tmp = mem_alloc(512);

			moda = l1 + ( (sc1_1 - sc1_3) / ( (sc1_1 - sc1_3) + (sc1_1 - sc1_2) ) ) * 0.005;
			sprintf(tmp, "./rrd_update2.sh %s_%s_CAT1 %ld %3.3f %3.3f %3.3f %3.3f %3.3f", sac_s, sic_s, 
			    time_old + t3, 
			    radar_delay[i].max_retardo_cat1,
			    radar_delay[i].min_retardo_cat1,
			    radar_delay[i].suma_retardos_cat1/radar_delay[i].cuenta_plot_cat1,
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat1 / radar_delay[i].cuenta_plot_cat1) - pow(radar_delay[i].suma_retardos_cat1 / radar_delay[i].cuenta_plot_cat1,2))
			    );
			system(tmp);
			moda = l2 + ( (sc2_1 - sc2_3) / ( (sc2_1 - sc2_3) + (sc2_1 - sc2_2) ) ) * 0.005;
			sprintf(tmp, "./rrd_update2.sh %s_%s_CAT2 %ld %3.3f %3.3f %3.3f %3.3f %3.3f", sac_s, sic_s, 
			    time_old + t3, 
			    radar_delay[i].max_retardo_cat2,
			    radar_delay[i].min_retardo_cat2,
			    radar_delay[i].suma_retardos_cat2/radar_delay[i].cuenta_plot_cat2,
			    (moda < -7.994) || (moda > 7.996) ? 0 : moda,
			    sqrt((radar_delay[i].suma_retardos_cuad_cat2 / radar_delay[i].cuenta_plot_cat2) - pow(radar_delay[i].suma_retardos_cat2 / radar_delay[i].cuenta_plot_cat2,2))
			    );
			system(tmp);
			mem_free(tmp);
    			exit(EXIT_SUCCESS);
		    }
		    mem_free(sac_s);
		    mem_free(sic_s);
		}
	    }
	    radar_delay_clear();
	}
    }	    

    exit(EXIT_SUCCESS);
}

