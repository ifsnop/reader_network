               :#include "includes.h"
               :
               :#define SERVER_TIMEOUT_SEC 10
               :
               :struct radar_delay_s {
               :    unsigned char sac,sic;
               :    long cuenta_plot_cat1, cuenta_plot_cat2;
               :    long cuenta_plot_cat8, cuenta_plot_cat10;
               :    float suma_retardos_cat1, suma_retardos_cat2;
               :    float suma_retardos_cat8, suma_retardos_cat10;
               :    float suma_retardos_cuad_cat1, suma_retardos_cuad_cat2;
               :    float suma_retardos_cuad_cat8, suma_retardos_cuad_cat10;
               :    float max_retardo_cat1, max_retardo_cat2;
               :    float max_retardo_cat8, max_retardo_cat10;
               :    float min_retardo_cat1, min_retardo_cat2;
               :    float min_retardo_cat8, min_retardo_cat10;
               :    int *segmentos_cat1, *segmentos_cat2;
               :    int *segmentos_cat8, *segmentos_cat10;
               :    int segmentos_max_cat1, segmentos_max_cat2;
               :    int segmentos_max_cat8, segmentos_max_cat10;
               :    int segmentos_ptr_cat1, segmentos_ptr_cat2;
               :    int segmentos_ptr_cat8, segmentos_ptr_cat10;
               :};
               :
               :struct radar_delay_s *radar_delay;
               :
               :struct ip_mreq mreq;
               :struct sockaddr_in addr;
               :fd_set reader_set;
               :int s, yes = 1;
               :bool forced_exit = false;
               :
               :
               :void server_connect(void) {
               :
               :    FD_ZERO(&reader_set);
               :    memset(&addr, 0, sizeof(addr));
               :    addr.sin_family = AF_INET;
               :    addr.sin_addr.s_addr = htonl(INADDR_ANY);
               :    addr.sin_port = htons(MULTICAST_PLOTS_PORT);
               :    if ( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
               :	log_printf(LOG_ERROR, "socket %s\n", strerror(errno));
               :	exit(1);
               :    }
               :    FD_SET(s, &reader_set);
               :    
               :    if ( bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
               :	log_printf(LOG_ERROR, "bind %s\n", strerror(errno));
               :	exit(1);
               :    }
               :    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
               :				
               :    mreq.imr_interface.s_addr = inet_addr("127.0.0.1"); //nHostAddress;
               :    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_PLOTS_GROUP);
               :    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
               :	log_printf(LOG_ERROR, "setsocktperror\n");
               :	exit(1);
               :    }
               :
               :    return;
               :}
               :
               :void radar_delay_alloc(void) {
               :int i;
               :
               :    radar_delay = (struct radar_delay_s *) mem_alloc(sizeof(struct radar_delay_s)*MAX_RADAR_NUMBER);
               :    for (i=0; i < MAX_RADAR_NUMBER; i++) {
               :	radar_delay[i].segmentos_cat1 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
               :	radar_delay[i].segmentos_cat2 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
               :	radar_delay[i].segmentos_cat8 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
               :	radar_delay[i].segmentos_cat10 = (int *) mem_alloc(sizeof(int) * MAX_SEGMENT_NUMBER);
               :    }
               :    return;
               :}
               :
               :void radar_delay_clear(void) {
               :int i;
               :    for (i=0; i < MAX_RADAR_NUMBER; i++) {
               :	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
               :	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
               :	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
               :	bzero(radar_delay[i].segmentos_cat1, sizeof(int) * MAX_SEGMENT_NUMBER);
               :    	radar_delay[i].sac = '\0'; radar_delay[i].sic = '\0';
               :	radar_delay[i].cuenta_plot_cat1 = 0; radar_delay[i].cuenta_plot_cat2 = 0;
               :	radar_delay[i].cuenta_plot_cat8 = 0; radar_delay[i].cuenta_plot_cat10 = 0;
               :	radar_delay[i].suma_retardos_cat1 = 0; radar_delay[i].suma_retardos_cat2 = 0;
               :	radar_delay[i].suma_retardos_cat8 = 0; radar_delay[i].suma_retardos_cat10 = 0;
               :	radar_delay[i].suma_retardos_cuad_cat1 = 0; radar_delay[i].suma_retardos_cuad_cat2 = 0;
               :	radar_delay[i].suma_retardos_cuad_cat8 = 0; radar_delay[i].suma_retardos_cuad_cat10 = 0;
               :	radar_delay[i].max_retardo_cat1 = -10000.0; radar_delay[i].max_retardo_cat2 = -10000.0;
               :	radar_delay[i].max_retardo_cat8 = -10000.0; radar_delay[i].max_retardo_cat10 = -10000.0;
               :	radar_delay[i].min_retardo_cat1 = 10000.0; radar_delay[i].min_retardo_cat2 = 10000.0;
               :        radar_delay[i].min_retardo_cat8 = 10000.0; radar_delay[i].min_retardo_cat10 = 10000.0;
               :	radar_delay[i].segmentos_max_cat1 = 0; radar_delay[i].segmentos_max_cat2 = 0;
               :	radar_delay[i].segmentos_max_cat8 = 0; radar_delay[i].segmentos_max_cat10 = 0;
               :	radar_delay[i].segmentos_ptr_cat1 = 0; radar_delay[i].segmentos_ptr_cat2 = 0;
               :	radar_delay[i].segmentos_ptr_cat8 = 0; radar_delay[i].segmentos_ptr_cat10 = 0;
               :    }
               :}
               :
               :void radar_delay_free(void) {
               :int i;
               :
               :    for (i=0; i < MAX_RADAR_NUMBER; i++) {
               :	mem_free(radar_delay[i].segmentos_cat1);
               :	mem_free(radar_delay[i].segmentos_cat2);
               :	mem_free(radar_delay[i].segmentos_cat8);
               :	mem_free(radar_delay[i].segmentos_cat10);
               :    }
               :    mem_free(radar_delay);
               :}
               :
               :int main(int argc, char *argv[]) {
               :    struct datablock_plot dbp;
               :    int dbplen, addrlen, i, j;
               :    struct timeval t,t2;
               :
               :    startup();
               :    
               :    log_printf(LOG_NORMAL, "init...\n");
               :    
               :    server_connect();
               :    radar_delay_alloc();
               :    radar_delay_clear();
               :    
               :    if (gettimeofday(&t, NULL) == -1) {
               :	log_printf(LOG_ERROR, "gettimeofday %s\n", strerror(errno));
               :	exit(EXIT_FAILURE);
               :    }
               :
    31  1.5233 :    while (!forced_exit) {
               :        struct timeval timeout;
     4  0.1966 :	float diff = 0.0;
               :	int select_return;
     9  0.4423 :	dbplen = sizeof(dbp);
     1  0.0491 :	addrlen = sizeof(addr);
               :	
               :
     4  0.1966 :	timeout.tv_sec = SERVER_TIMEOUT_SEC;
    23  1.1302 :	timeout.tv_usec = 0;
    73  3.5872 :	select_return = select(s+1, &reader_set, NULL, NULL, &timeout);
               :	
     8  0.3931 :	if (select_return > 0) {
     8  0.3931 :	    FD_SET(s, &reader_set);		
    62  3.0467 :	    if (recvfrom(s, &dbp, dbplen, 0, (struct sockaddr *) &addr, &addrlen) < 0) {
               :		log_printf(LOG_ERROR, "recvfrom\n");
               :		exit(EXIT_FAILURE);
               :	    }    
    19  0.9337 :	    if (dbp.cat == CAT_255) {
               :		log_printf(LOG_ERROR, "fin de fichero\n");
               :		forced_exit = true;
               :	    } 
    25  1.2285 :	    if (dbp.available & IS_TOD) {
   164  8.0590 :		diff = dbp.tod_stamp - dbp.tod;
     2  0.0983 :		i=0;
               :		while ( (i < MAX_RADAR_NUMBER) && 
               :		    ( (dbp.sac != radar_delay[i].sac) ||
               :		    (dbp.sic != radar_delay[i].sic) ) &&
               :		    (radar_delay[i].sac != 0) &&
    45  2.2113 :		    (radar_delay[i].sic != 0) ) { 
               :		    i++; 
               :		}
               :
    39  1.9165 :		if (i == MAX_RADAR_NUMBER) {
               :		    log_printf(LOG_ERROR, "no hay suficientes slots para radares\n");
               :		    exit(EXIT_FAILURE);
               :		} else {
    23  1.1302 :		    if ( (!radar_delay[i].sac) &&
               :			(!radar_delay[i].sic) ) {
               :			radar_delay[i].sac = dbp.sac;
               :			radar_delay[i].sic = dbp.sic;
               :		    }
               :		}
               :
    20  0.9828 :		switch (dbp.cat) {
               :		case CAT_01 : { 
    10  0.4914 :				radar_delay[i].cuenta_plot_cat1++;
    18  0.8845 :			        radar_delay[i].suma_retardos_cat1+=diff;
    61  2.9975 :				radar_delay[i].suma_retardos_cuad_cat1+=pow(diff,2);
    25  1.2285 :				if (diff > radar_delay[i].max_retardo_cat1)
               :				    radar_delay[i].max_retardo_cat1 = diff;
    29  1.4251 :				if (diff < radar_delay[i].min_retardo_cat1)
               :				    radar_delay[i].min_retardo_cat1 = diff;
    37  1.8182 :				if (fabs(diff) < 16.0) {
   200  9.8280 :				    radar_delay[i].segmentos_cat1[(int) floorf((diff+8.0)*10000.0/50.0)]++;
    53  2.6044 :				} else {
               :				    log_printf(LOG_ERROR, "retardo mayor de 8 segundos\n");
               :				    exit(EXIT_FAILURE);
               :				}				 
     4  0.1966 :				break; }
               :		case CAT_02 : { 
     1  0.0491 :				log_printf(LOG_NORMAL,"%02X %02X (%3.3f)\n", radar_delay[i].sac, 
               :				    radar_delay[i].sic, diff);
               :
               :				radar_delay[i].cuenta_plot_cat2++;
     3  0.1474 :			        radar_delay[i].suma_retardos_cat2+=diff;
               :				radar_delay[i].suma_retardos_cuad_cat2+=pow(diff,2);
     3  0.1474 :				if (diff > radar_delay[i].max_retardo_cat2)
               :				    radar_delay[i].max_retardo_cat2 = diff;
     1  0.0491 :				if (diff < radar_delay[i].min_retardo_cat2)
               :				    radar_delay[i].min_retardo_cat2 = diff;
     2  0.0983 :				if (fabs(diff) < 16.0) {
     6  0.2948 :				    radar_delay[i].segmentos_cat2[(int) floorf((diff+8.0)*10000.0/50.0)]++;
     3  0.1474 :				} else {
               :				    log_printf(LOG_ERROR, "retardo mayor de 8 segundos\n");
               :				    exit(EXIT_FAILURE);
               :				}				 
               :				break; }
               :		case CAT_08 : { 
               :				radar_delay[i].cuenta_plot_cat8++;
               :			        radar_delay[i].suma_retardos_cat8+=diff;
               :				radar_delay[i].suma_retardos_cuad_cat8+=pow(diff,2);
               :				if (diff > radar_delay[i].max_retardo_cat8)
               :				    radar_delay[i].max_retardo_cat8 = diff;
     1  0.0491 :				if (diff < radar_delay[i].min_retardo_cat8)
               :				    radar_delay[i].min_retardo_cat8 = diff;
               :				if (fabs(diff) < 16.0) {
     3  0.1474 :				    radar_delay[i].segmentos_cat8[(int) floorf((diff+8.0)*10000.0/50.0)]++;
     1  0.0491 :				} else {
               :				    log_printf(LOG_ERROR, "retardo mayor de 8 segundos\n");
               :				    exit(EXIT_FAILURE);
               :				}
               :				break; }
               :		case CAT_10 : { 
               :				radar_delay[i].cuenta_plot_cat10++;
               :			        radar_delay[i].suma_retardos_cat10+=diff;
               :				radar_delay[i].suma_retardos_cuad_cat10+=pow(diff,2);
               :				if (diff > radar_delay[i].max_retardo_cat10)
               :				    radar_delay[i].max_retardo_cat10 = diff;
               :				if (diff < radar_delay[i].min_retardo_cat10)
               :				    radar_delay[i].min_retardo_cat10 = diff;
               :				if (fabs(diff) < 16.0) {
               :				    radar_delay[i].segmentos_cat10[(int) floorf((diff+8.0)*10000.0/50.0)]++;
               :				} else {
               :				    log_printf(LOG_ERROR, "retardo mayor de 8 segundos\n");
               :				    exit(EXIT_FAILURE);
               :				}				 
               :				break; }
               :		default : {
               :				log_printf(LOG_ERROR, "categoria desconocida %08X", dbp.cat);
               :				exit(EXIT_FAILURE);
               :				break; }
               :		}
               :	    }
     5  0.2457 :        } else if (select_return == 0) {
               :	    close(s);
               :	    server_connect();
               :/*
               :    	    log_printf(LOG_VERBOSE, "sin conexion\n");
               :	    mreq.imr_interface.s_addr = inet_addr("127.0.0.1"); //nHostAddress;
               :	    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_PLOTS_GROUP);
               :	    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
               :		log_printf(LOG_ERROR, "setsocktperror %s\n", strerror(errno));
               :		exit(1);
               :	    }
               :*/
               :	    timeout.tv_sec = SERVER_TIMEOUT_SEC;
               :	    timeout.tv_usec = 0;
               :	} else {
               :	    log_printf(LOG_ERROR, "socket error: %s\n", strerror(errno));
               :	    close(s);
               :	    server_connect();
               :	}
               :        
   135  6.6339 :	if (gettimeofday(&t2, NULL) == -1) {
               :	    log_printf(LOG_ERROR, "gettimeofday %s\n", strerror(errno));
               :	    exit(EXIT_FAILURE);
               :	}
               :
   161  7.9115 :	if ( ( ((float)t2.tv_sec + t2.tv_usec/1000000.0) -
               :	    ((float)t.tv_sec + t.tv_usec/1000000.0) > UPDATE_TIME ) ||
               :	    forced_exit) {
               :	    char *sac_s=0, *sic_l=0;
               :	    float l1=0.0, l2=0.0, l8=0.0, l10=0.0;
               :	    float sc1_1=0.0, sc1_2=0.0, sc1_3=0.0;
               :    	    float sc2_1=0.0, sc2_2=0.0, sc2_3=0.0;
               :	    float sc8_1=0.0, sc8_2=0.0, sc8_3=0.0;
               :	    float sc10_1=0.0, sc10_2=0.0, sc10_3=0.0;
               :	    float moda;
               :	
               :	    log_printf(LOG_NORMAL, "RADAR\t\t\tCAT\tplots\tmedia\tdesv\tmoda\tmax\tmin\n");
               :
               :	    for(i=0; i<MAX_RADAR_NUMBER; i++) {
    42  2.0639 :	        for(j=0; j<3200; j++) {
   197  9.6806 :    	    	    if (radar_delay[i].segmentos_cat1[j] > radar_delay[i].segmentos_max_cat1) {
               :		        radar_delay[i].segmentos_ptr_cat1 = j;
               :			radar_delay[i].segmentos_max_cat1 = radar_delay[i].segmentos_cat1[j];
               :		    }
   140  6.8796 :		    if (radar_delay[i].segmentos_cat2[j] > radar_delay[i].segmentos_max_cat2) {
               :		        radar_delay[i].segmentos_ptr_cat2 = j;
               :		        radar_delay[i].segmentos_max_cat2 = radar_delay[i].segmentos_cat2[j];
               :		    }
   123  6.0442 :		    if (radar_delay[i].segmentos_cat8[j] > radar_delay[i].segmentos_max_cat8) {
               :		        radar_delay[i].segmentos_ptr_cat8 = j;
               :		        radar_delay[i].segmentos_max_cat8 = radar_delay[i].segmentos_cat8[j];
               :		    }
   137  6.7322 :		    if (radar_delay[i].segmentos_cat10[j] > radar_delay[i].segmentos_max_cat10) {
               :		        radar_delay[i].segmentos_ptr_cat10 = j;
               :		        radar_delay[i].segmentos_max_cat10 = radar_delay[i].segmentos_cat10[j];
               :		    }
               :		}
               :		
               :		l1 =    ( ((float) radar_delay[i].segmentos_ptr_cat1) / 10000.0*50.0 ) - 8.0;
               :		sc1_1 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1]) / 10000.0*50.0 ) - 8.0;
               :		sc1_2 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1 + 1]) / 10000.0*50.0 ) - 8.0;
     1  0.0491 :		sc1_3 = ( ((float) radar_delay[i].segmentos_cat1[radar_delay[i].segmentos_ptr_cat1 - 1]) / 10000.0*50.0 ) - 8.0;
     1  0.0491 :	        l2 =    ( ((float) radar_delay[i].segmentos_ptr_cat2) / 10000.0*50.0 ) - 8.0;
               :	        sc2_1 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2]) / 10000.0*50.0 ) - 8.0;
               :	        sc2_2 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2 + 1]) / 10000.0*50.0 ) - 8.0;
     1  0.0491 :	        sc2_3 = ( ((float) radar_delay[i].segmentos_cat2[radar_delay[i].segmentos_ptr_cat2 - 1]) / 10000.0*50.0 ) - 8.0;
               :	        l8 =    ( ((float) radar_delay[i].segmentos_ptr_cat8) / 10000.0*50.0 ) - 8.0;
               :	        sc8_1 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8]) / 10000.0*50.0 ) - 8.0;
               :	        sc8_2 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8 + 1]) / 10000.0*50.0 ) - 8.0;
               :	        sc8_3 = ( ((float) radar_delay[i].segmentos_cat8[radar_delay[i].segmentos_ptr_cat8 - 1]) / 10000.0*50.0 ) - 8.0;
               :	        l10 =    ( ((float) radar_delay[i].segmentos_ptr_cat10) / 10000.0*50.0 ) - 8.0;
               :	        sc10_1 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10]) / 10000.0*50.0 ) - 8.0;
               :	        sc10_2 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10 + 1]) / 10000.0*50.0 ) - 8.0;
               :	        sc10_3 = ( ((float) radar_delay[i].segmentos_cat10[radar_delay[i].segmentos_ptr_cat10 - 1]) / 10000.0*50.0 ) - 8.0;
               :
               :		if (radar_delay[i].sac && radar_delay[i].sic) {
               :        	    sac_s = ast_get_SACSIC((unsigned char *) &radar_delay[i].sac,
               :			    (unsigned char *) &radar_delay[i].sic, GET_SAC_SHORT);
               :		    sic_l = ast_get_SACSIC((unsigned char *) &radar_delay[i].sac, 
               :			    (unsigned char *) &radar_delay[i].sic, GET_SIC_LONG);
               :		    moda = l1 + ( (sc1_1 - sc1_3) / ( (sc1_1 - sc1_3) + (sc1_1 - sc1_2) ) ) * 0.005;
               :		    log_printf(LOG_NORMAL, "%s %s\t1\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
               :			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat1,
               :			    radar_delay[i].suma_retardos_cat1/radar_delay[i].cuenta_plot_cat1,
               :			    sqrt((radar_delay[i].suma_retardos_cuad_cat1 / radar_delay[i].cuenta_plot_cat1) - pow(radar_delay[i].suma_retardos_cat1 / radar_delay[i].cuenta_plot_cat1,2)),
               :			    (moda < -7.994) && (moda > -7.996) ? 0 : moda,
               :		    	    radar_delay[i].max_retardo_cat1 == -10000 ? 0 : radar_delay[i].max_retardo_cat1,
               :			    radar_delay[i].min_retardo_cat1 ==  10000 ? 0 : radar_delay[i].min_retardo_cat1);
               :	    	    moda = l2 + ( (sc2_1 - sc2_3) / ( (sc2_1 - sc2_3) + (sc2_1 - sc2_2) ) ) * 0.005;
               :		    log_printf(LOG_NORMAL, "%s %s\t2\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
               :			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat2,
               :			    radar_delay[i].suma_retardos_cat2/radar_delay[i].cuenta_plot_cat2,
               :			    sqrt((radar_delay[i].suma_retardos_cuad_cat2 / radar_delay[i].cuenta_plot_cat2) - pow(radar_delay[i].suma_retardos_cat2 / radar_delay[i].cuenta_plot_cat2,2)),
               :			    (moda < -7.993) && (moda > -7.996) ? 0 : moda,
               :		    	    radar_delay[i].max_retardo_cat2 == -10000 ? 0 : radar_delay[i].max_retardo_cat2,
               :			    radar_delay[i].min_retardo_cat2 ==  10000 ? 0 : radar_delay[i].min_retardo_cat2);
               :		    moda = l8 + ( (sc8_1 - sc8_3) / ( (sc8_1 - sc8_3) + (sc8_1 - sc8_2) ) ) * 0.005;
               :		    log_printf(LOG_NORMAL, "%s %s\t8\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
               :			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat8,
               :			    radar_delay[i].suma_retardos_cat8/radar_delay[i].cuenta_plot_cat8,
               :			    sqrt((radar_delay[i].suma_retardos_cuad_cat8 / radar_delay[i].cuenta_plot_cat8) - pow(radar_delay[i].suma_retardos_cat8 / radar_delay[i].cuenta_plot_cat8,2)),
               :			    (moda < -7.994) && (moda > -7.996) ? 0 : moda,
               :		    	    radar_delay[i].max_retardo_cat8 == -10000 ? 0 : radar_delay[i].max_retardo_cat8,
               :			    radar_delay[i].min_retardo_cat8 ==  10000 ? 0 : radar_delay[i].min_retardo_cat8);
               :	    	    moda = l10 + ( (sc10_1 - sc10_3) / ( (sc10_1 - sc10_3) + (sc10_1 - sc10_2) ) ) * 0.005;
               :		    log_printf(LOG_NORMAL, "%s %s\t10\t%ld\t%3.3f\t%3.3f\t%3.3f\t%3.3f\t%3.3f\n",
               :			    sac_s, sic_l, radar_delay[i].cuenta_plot_cat10,
               :			    radar_delay[i].suma_retardos_cat10/radar_delay[i].cuenta_plot_cat10,
               :			    sqrt((radar_delay[i].suma_retardos_cuad_cat10 / radar_delay[i].cuenta_plot_cat10) - pow(radar_delay[i].suma_retardos_cat10 / radar_delay[i].cuenta_plot_cat10,2)),
               :			    (moda < -7.994) && (moda > -7.996) ? 0 : moda,
               :		    	    radar_delay[i].max_retardo_cat10 == -10000 ? 0 : radar_delay[i].max_retardo_cat10,
               :			    radar_delay[i].min_retardo_cat10 ==  10000 ? 0 : radar_delay[i].min_retardo_cat10);
               :		    log_printf(LOG_NORMAL, "-----------------------------------------------------------------------------\n");
               :		    mem_free(sac_s);
               :		    mem_free(sic_l);
               :		}
               :	    }	
               :	    t.tv_sec = t2.tv_sec; t.tv_usec = t2.tv_usec;
               :	    radar_delay_clear();
               :	}
    54  2.6536 :	log_flush();
     2  0.0983 :    }
               :
               :    log_printf(LOG_NORMAL, "end...\n");
               :    log_flush();
               :
               :    exit(EXIT_SUCCESS);
               :}
               :
/* 
 * Total samples for file : "/root/prog/conversor_asterix2/src/client_time.c"
 * 
 *   2020 99.2629
 */


/* 
 * Command line: opannotate --source --output-dir=anot2 ./client_time 
 * 
 * Interpretation of command line:
 * Output annotated source file with samples
 * Output all files
 * 
 * CPU: PIII, speed 747.746 MHz (estimated)
 * Counted CPU_CLK_UNHALTED events (clocks processor is not halted) with a unit mask of 0x00 (No unit mask) count 6000
 */
