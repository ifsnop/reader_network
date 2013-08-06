#define ESC_STR "\033["
#define RED     "1;31"
#define GREEN   "1;32"
#define YELLOW  "1;33"
#define BLUE    "1;34"
#define MAGENTA "1;35"
#define CYAN    "1;36"
#define WHITE   "1;37"
#define BROWN   "40;33"


#include "includes.h"

extern float current_time;
extern int s;
extern struct sockaddr_in srvaddr;

void ast_output_datablock(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, unsigned long index) {
int i;
char *ptr_tmp;

    ptr_tmp = (char *) mem_alloc(size_datablock*3 + 2);
    memset(ptr_tmp, 0x0, size_datablock*3 + 2);
    
    for (i=0; i < size_datablock*3; i+=3)
	sprintf((char *)(ptr_tmp + i), "%02X ", (unsigned char) (ptr_raw[i/3]));

    if (id)
	log_printf(LOG_VERBOSE, "%ld%c %s\n", id, index != 0 ? (int)(index + 97) : 32, ptr_tmp);
    else
	log_printf(LOG_VERBOSE, ESC_STR CYAN"m%s"ESC_STR"0m\n", ptr_tmp);
//	printf(ESC_STR RED"m%ld%c %s"ESC_STR"0m\n", id, index != 0 ? (int)(index + 97) : 32, ptr_tmp);

    mem_free(ptr_tmp);
    return;
}

int ast_get_size_FSPEC(unsigned char *ptr_raw, ssize_t size_datablock) {
int sizeFSPEC = 0;

    //no distincion entre plot y track
    if ( size_datablock <= 3 ) return -1;
	
    if ( ptr_raw[0] & 1 ) {
	if ( size_datablock < 5) { return -1; }
	if ( ptr_raw[1] & 1 ) {
    	    if ( size_datablock < 6) { return -1; }
	    if ( ptr_raw[2] & 1 ) {
		if ( size_datablock < 7 ) { return -1; }
		sizeFSPEC = 4;
    	    } else {
		sizeFSPEC = 3;
    	    }
        } else {
    	    sizeFSPEC = 2;
	}
    } else  {
	sizeFSPEC = 1;
    }
    
    return sizeFSPEC;
}

int ast_procesarCAT01(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id) {
int size_current = 0, j = 0;
int index = 0;
    do {
	int sizeFSPEC;
	struct datablock_plot dbp;

//	log_printf(LOG_NORMAL, "fspec %02X\n", ptr_raw[0]);

	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
	dbp.cat = CAT_01;
	dbp.available = IS_ERROR;
	dbp.type = NO_DETECTION;
        dbp.modea_status = 0;
        dbp.modec_status = 0;
	dbp.flag_test = 0;
	dbp.tod_stamp = current_time; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;
    
	if (sizeFSPEC == 0) {
	    log_printf(LOG_WARNING, "ERROR_FSPEC_SIZE[%d] %s\n", sizeFSPEC, ptr_raw);
	    return T_ERROR;
	}
	
	j = sizeFSPEC;
	size_current += sizeFSPEC;
	if ( ptr_raw[0] & 128 ) { //I001/010
	    dbp.sac = ptr_raw[sizeFSPEC];
	    dbp.sic = ptr_raw[sizeFSPEC + 1];
	    j += 2; size_current += 2;
	    dbp.available |= IS_SACSIC;
	}
	if ( ptr_raw[0] & 64 ) { //I001/010
	    if ( ptr_raw[j] & 128 ) { // track
	        dbp.available |= IS_TRACK;
		size_current = size_datablock - 3; //exit without further decompression
	    
	    } else { // plot
		dbp.available |= IS_TYPE;
//		log_printf(LOG_NORMAL, "type %02X\n", ptr_raw[j]);

		if ( (!(ptr_raw[j] & 32)) && (!(ptr_raw[j] & 16)) ) {
		    dbp.type = NO_DETECTION;
		} else if ( (!(ptr_raw[j] & 32)) && (ptr_raw[j] & 16) ) {
		    dbp.type = TYPE_C1_PSR;
		} else if ( (ptr_raw[j] & 32) && (!(ptr_raw[j] & 16)) ) {
		    dbp.type = TYPE_C1_SSR;
		} else if ( (ptr_raw[j] & 32) && (ptr_raw[j] & 16) ) {
		    dbp.type = TYPE_C1_CMB;
		}
		if ( ptr_raw[j] & 2 ) {
		    dbp.type |= FROM_C1_FIXED_TRANSPONDER;
		}
		if (ptr_raw[j] & 1) {
		    j++; size_current++;
		    if (ptr_raw[j] & 128) { dbp.flag_test = 1; }
		    while (ptr_raw[j] & 1) { j++; size_current++;}
		}
		size_current++; j++;
		if ( ptr_raw[0] & 32 ) { //I001/040
//		    log_printf(LOG_NORMAL, "polar %02X %02X %02X %02X \n", ptr_raw[j], ptr_raw[j+1], ptr_raw[j+2], ptr_raw[j+3] );
	    	    dbp.rho = (ptr_raw[j]*256 + ptr_raw[j+1]) / 128.0;
	    	    dbp.theta = (ptr_raw[j+2]*256 + ptr_raw[j+3]) * 360.0/65536.0;
	    	    size_current += 4; j+= 4;
	    	    dbp.available |= IS_MEASURED_POLAR;
	        }
		if ( ptr_raw[0] & 16 ) { //I001/070
//		    log_printf(LOG_NORMAL, "modea %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
		    dbp.modea_status |= (ptr_raw[j] & 128) ? STATUS_MODEA_NOTVALIDATED : 0;
		    dbp.modea_status |= (ptr_raw[j] & 64) ? STATUS_MODEA_GARBLED : 0;
		    dbp.modea_status |= (ptr_raw[j] & 32) ? STATUS_MODEA_SMOOTHED : 0;
		    dbp.modea = (ptr_raw[j] & 15)*256 + ptr_raw[j+1];
		    size_current += 2; j += 2;
		    dbp.available |= IS_MODEA;
		}
		if ( ptr_raw[0] & 8  ) { //I001/090
//		    log_printf(LOG_NORMAL, "modec %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
		    dbp.modec_status |= (ptr_raw[j] & 128) ? STATUS_MODEC_NOTVALIDATED : 0;
		    dbp.modec_status |= (ptr_raw[j] & 64) ? STATUS_MODEC_GARBLED : 0;
		    dbp.modec = ( (ptr_raw[j] & 63)*256 + ptr_raw[j+1]) * 1.0/4.0;// * 100;
		    size_current += 2; j += 2;
		    dbp.available |= IS_MODEC;
		}
		if ( ptr_raw[0] & 4  ) { //I001/130
//		    log_printf(LOG_NORMAL, "responses %02X\n", ptr_raw[j]);
	    	    dbp.radar_responses = (ptr_raw[j] >> 1);
		    while (ptr_raw[j] & 1) { j++; size_current++; }
		    size_current++; j++;
		    dbp.available |= IS_RADAR_RESPONSES;
		}
		if ( ptr_raw[0] & 2  ) { //I001/141
//		    log_printf(LOG_NORMAL, "tod %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
	    	    dbp.truncated_tod = (ptr_raw[j]*256 + ptr_raw[j+1]) / 128.0;
		    if ( (dbp.tod = ttod_get_full(dbp.sac, dbp.sic, ptr_raw + j)) != T_ERROR )
			dbp.available |= IS_TOD;
		    size_current += 2; j += 2;
		    dbp.available |= IS_TRUNCATED_TOD;
		}
		if ( (sizeFSPEC > 1) && (ptr_raw[1] & 16) ) { //I001/080
		    size_current += 2; j += 2;
		}
		if ( (sizeFSPEC > 1) && (ptr_raw[1] & 8) ) { //I001/100
		    size_current += 4; j += 4;
		}
	    }
	}
//	ast_output_datablock(ptr_raw, j , dbp.id, dbp.index);
//	if ( (dbp.available & IS_TYPE) && (dbp.available & IS_TOD) ) {
	if ( dbp.available & IS_TYPE ) {
/*	    log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
	    if ((dbp.tod_stamp - dbp.tod < 0) || ((dbp.tod_stamp - dbp.tod > 5)) ) {
		log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);	
		exit(EXIT_FAILURE);
	    }
*/	    if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
	    }
	}
	
        ptr_raw += j;
	index++;
    } while ((size_current + 3) < size_datablock);

    return T_OK;
}

int ast_procesarCAT02(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id) {
int sizeFSPEC=0, pos=0;
struct datablock_plot dbp;

    dbp.tod_stamp = current_time;
    dbp.flag_test = 0;
    dbp.id = id;
    dbp.index = 0;
    sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
//    ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);

    if ( (ptr_raw[0] & 128) && // sac/sic
	 (ptr_raw[0] & 64) &&  // msg type
	 (ptr_raw[0] & 16) ) { // timeofday
	
	if (ptr_raw[0] & 32) pos++;
	
	pos += sizeFSPEC + 3; //FSPEC SACSIC MSGTYPE
	ttod_put_full(ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC + 1], ptr_raw + pos);

	dbp.cat = CAT_02;
	dbp.sac = ptr_raw[sizeFSPEC]; dbp.sic = ptr_raw[sizeFSPEC+1];
	dbp.available = IS_TOD | IS_TYPE | IS_SACSIC;
	dbp.tod = ((float)(ptr_raw[pos]*256*256 + ptr_raw[pos+1]*256 + ptr_raw[pos+2]))/128.0;
	switch(ptr_raw[sizeFSPEC+2]) {
	    case 1: dbp.type = TYPE_C2_NORTH_MARKER;			break;
	    case 2: dbp.type = TYPE_C2_SECTOR_CROSSING;			break;
	    case 3: dbp.type = TYPE_C2_SOUTH_MARKER;			break;
	    case 8: dbp.type = TYPE_C2_START_BLIND_ZONE_FILTERING;	break;
	    case 9: dbp.type = TYPE_C2_STOP_BLIND_ZONE_FILTERING;	break;
	    default: dbp.type = NO_DETECTION;				break;
	}
	if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
	    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
	}
    }
    return T_OK;
}

int ast_procesarCAT08(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id) {
int sizeFSPEC=0;
struct datablock_plot dbp;

    dbp.tod_stamp = current_time; dbp.id = id; dbp.index = 0;
    sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
//    ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);

    if ( (ptr_raw[0] & 128) && // sac/sic
	(ptr_raw[0] & 64) &&   // msg type
	(ptr_raw[0] & 1) &&    // FX
	(ptr_raw[1] & 128) ) { // timeofday

	dbp.cat = CAT_08;
	dbp.type = NO_DETECTION;
	dbp.sac = ptr_raw[sizeFSPEC]; dbp.sic = ptr_raw[sizeFSPEC+1];
	
	if (ptr_raw[sizeFSPEC + 2] & 254) {
	    dbp.type = TYPE_C8_CONTROL_SOP;
	}
	if (ptr_raw[sizeFSPEC + 2] & 255) {
	    dbp.type = TYPE_C8_CONTROL_EOP;
	}
	if ( (dbp.type == TYPE_C8_CONTROL_SOP) || 
	     (dbp.type == TYPE_C8_CONTROL_EOP) ) {
	    //start of picture or end of picture
	    dbp.available = IS_TOD | IS_TYPE | IS_SACSIC;
	    dbp.tod = ((float)(ptr_raw[sizeFSPEC + 3]<<16) + 
			(ptr_raw[sizeFSPEC + 4]<<8) + 
			(ptr_raw[sizeFSPEC + 5])) / 128.0;
		
	    if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	    }
	}
    }
    return T_OK;
}

void printTOD() {
    struct timeval t;
    if (gettimeofday(&t, NULL)==0) {
        log_printf(LOG_VERBOSE, "%s", ctime(&t.tv_sec));
    } else {
        log_printf(LOG_VERBOSE, "error gettimeofday %s\n", strerror(errno));
    }
    return;			
}

void ttod_put_full(unsigned char sac, unsigned char sic, unsigned char *ptr_full_tod) {
int i=0;

    while ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) &&
	    (full_tod[i] != 0) &&
	    (full_tod[i+1] != 0) &&
	    (full_tod[i] != sac) && 
	    (full_tod[i+1] != sic) )
	    i+=TTOD_WIDTH;

    if ( i < MAX_RADAR_NUMBER*TTOD_WIDTH ) {
	if (full_tod[i] == 0) {
	    full_tod[i] = sac;
	    full_tod[i+1] = sic;
	}
	full_tod[i+2] = ptr_full_tod[0];
	full_tod[i+3] = ptr_full_tod[1];
	full_tod[i+4] = ptr_full_tod[2];
	
	full_tod[i+5] = ptr_full_tod[1];
	full_tod[i+6] = ptr_full_tod[2];
    }
    return;
}

float ttod_get_full(int sac, int sic, unsigned char *ptr_ttod) {
int i = 0;
int adjust = 0;

    while ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) &&
	    (full_tod[i] != 0) &&
	    (full_tod[i+1] != 0) &&
	    (full_tod[i] != (unsigned char) sac) && 
	    (full_tod[i+1] != (unsigned char) sic) )
	    i+=TTOD_WIDTH;

    if ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) && (full_tod[i] != 0) && (full_tod[i+1] != 0) ) {
	long p = (full_tod[i+5]<<8) + full_tod[i+6];
	long q = (ptr_ttod[0]<<8) + ptr_ttod[1];
	long d = abs(q-p);

	//printTOD();		
	//log_printf(LOG_VERBOSE, "SAC: %d SIC: %d\n", sac, sic);
	//log_printf(LOG_VERBOSE, "\t>NRT %02X%02X%02X %d\n", full_tod[i+2], full_tod[i+3], full_tod[i+4],
	//	(full_tod[i+2]<<16) + (full_tod[i+3]<<8) + (full_tod[i+4]) );
	//log_printf(LOG_VERBOSE, "\tPLT    %02X%02X p:%ld q:%ld q-p:%d\n", ptr_ttod[0], ptr_ttod[1], p, q, abs(q-p) );
	
	if ( d <= 16*128 ) {		// dentro de la ventana de seguimiento (16 segundos, 2 vueltas antena)
	    full_tod[i+5] = ptr_ttod[0];
	    full_tod[i+6] = ptr_ttod[1];
	    //log_printf(LOG_VERBOSE, "\tADJ1 %02X%02X%02X %ld/128.0\n", full_tod[i+2], ptr_ttod[0], ptr_ttod[1], (full_tod[i+2]<<16) + q);
	} else if ( (d > 0xB800) && (d <= 0xC000) && (full_tod[i+2] == 0xA8) ) {// fuera de ventana de seguimiento
	    // rollover del tod. cuando pasan las 00:00:00, el partial tod de cat1 se reinicia desde 0,
	    // pero al no existir paso por norte, lo que queda en full_tod[i+2] no vale, tendría que ser 0.
	    // se ajusta "adjust" astutamente para que la suma de 0.
	    // el valor de full_tod[i+2] antes de las 00:00:00 es de A8,
	    // viene de que a8c0000 es el num max de segs en un dia
            adjust=-full_tod[i+2];
	    //log_printf(LOG_VERBOSE, "\tADJ2 %02X%02X%02X %ld/128.0\n", full_tod[i+2] + adjust, ptr_ttod[0], ptr_ttod[1], ((full_tod[i+2]+adjust)<<16) + q);
        } else if ( d > 0xF800 ) { 	// se ha dado la vuelta al contador, respetamos 0xFFFF - 16*128
	    if ( (q-p) > 0 ) { 		// es positivo si es un plot que llega tarde
		adjust = -1;
		    
		//Fri Jul  6 07:15:12 2007
		//    SAC: 20 SIC: 133
		//    NRT 330007 3342343
		//    PLT    FFED p:29 q:65517 q-p:65488
		//    ADJ3 32FFED 3342317/128.0
	    	//log_printf(LOG_VERBOSE, "\tADJ3 %02X%02X%02X %ld/128.0\n", full_tod[i+2] + adjust, ptr_ttod[0], ptr_ttod[1], ((full_tod[i+2]+adjust)<<16) + q);
	    } else {			// es negativo si todavia no ha llegado el paso por norte
		adjust = 1;
		//log_printf(LOG_VERBOSE, "\tADJ4 %02X%02X%02X %ld/128.0\n", full_tod[i+2] + adjust, ptr_ttod[0], ptr_ttod[1], ((full_tod[i+2]+adjust)<<16) + q);
	    }
        } else { //if ( (abs(q-p)>16*128) && (abs(q-p)<=60000) ) { // ERROR, fuera de ventanas de seguiemiento
	    //	SAC: 20 SIC: 133
	    //	NRT  2A1C0F 2759695
	    //	PLT    FFC3 p:7240 q:65475 q-p:58235
	    //	ADJ5 2AFFC3 2817987/128.0 ERROR
	    //	Sun Jul  8 05:59:20 2007
	    printTOD();
	    log_printf(LOG_ERROR, "\tSAC: %d SIC: %d\n", sac, sic);
	    log_printf(LOG_ERROR, "\tNRT  %02X%02X%02X %d\n", full_tod[i+2], full_tod[i+3], full_tod[i+4],
		(full_tod[i+2]<<16) + (full_tod[i+3]<<8) + (full_tod[i+4]) );
	    log_printf(LOG_ERROR, "\tPLT    %02X%02X p:%ld q:%ld q-p:%d\n", ptr_ttod[0], ptr_ttod[1], p, q, abs(q-p) );
	    log_printf(LOG_ERROR, "\tADJ5 %02X%02X%02X %ld/128.0 ERROR\n\n", full_tod[i+2], ptr_ttod[0], ptr_ttod[1], (full_tod[i+2]<<16) + q);
	    return T_ERROR;
	}
	return ((float)( ((full_tod[i+2] + adjust)<<16) + q))/128.0;
    }
    return T_ERROR;
};

