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
		if ( ptr_raw[3] & 1 ) {
		    if (size_datablock < 8) { return -1; }
		    sizeFSPEC = 5;
		} else {
		    sizeFSPEC = 4;
		}
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

int ast_procesarCAT01(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
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
	dbp.plot_type = IS_ERROR;
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
		    if ( (dbp.tod = ttod_get_full(dbp.sac, dbp.sic, ptr_raw + j, id)) != T_ERROR )
			dbp.available |= IS_TOD;
//		    else {
//			// IFSNOP
//			ast_output_datablock(ptr_raw, size_datablock - 3, id, dbp.index);
//		    }
		    size_current += 2; j += 2;
		    dbp.available |= IS_TRUNCATED_TOD;
		}
		if ( (sizeFSPEC > 1) && (ptr_raw[1] & 32) ) { //I001/131
		    size_current += 1; j += 1;
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
*/	    if (enviar) {
		if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		}
	    } else {
		update_calculations(dbp);
	    }
	}
	
        ptr_raw += j;
	index++;
    } while ((size_current + 3) < size_datablock);

    return T_OK;
}

int ast_procesarCAT02(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int sizeFSPEC=0, pos=0;
struct datablock_plot dbp;

    dbp.plot_type = IS_ERROR;
    dbp.tod_stamp = current_time;
    dbp.flag_test = 0;
    dbp.id = id;
    dbp.index = 0;
    sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);

//    log_printf(LOG_ERROR, "a)CAT02] (%d) %02X %02X\n", sizeFSPEC, ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC+1] );

    if ( (ptr_raw[0] & 128) && // sac/sic
	 (ptr_raw[0] & 64) &&  // msg type
	 (ptr_raw[0] & 16) ) { // timeofday
    
//	log_printf(LOG_ERROR, "b)CAT02] (%d) %02X %02X \n", sizeFSPEC, ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC+1] );

	if (ptr_raw[0] & 32) pos++;
	
	pos += sizeFSPEC + 3; //FSPEC SACSIC MSGTYPE
	
//	log_printf(LOG_ERROR, "c)CAT02] (%d) %02X %02X \n", sizeFSPEC, ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC+1] );
	
	ttod_put_full(ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC + 1], ptr_raw + pos);

	dbp.cat = CAT_02;
	dbp.sac = ptr_raw[sizeFSPEC]; dbp.sic = ptr_raw[sizeFSPEC+1];
	dbp.available = IS_TOD | IS_TYPE | IS_SACSIC;
	dbp.tod = ((float)(ptr_raw[pos]*256*256 + ptr_raw[pos+1]*256 + ptr_raw[pos+2]))/128.0;
	//if (dbp.sac == 1 && dbp.sic==1) {
	//    ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);
	//}
	switch(ptr_raw[sizeFSPEC+2]) {
	    case 1: dbp.type = TYPE_C2_NORTH_MARKER;			break;
	    case 2: dbp.type = TYPE_C2_SECTOR_CROSSING;			break;
	    case 3: dbp.type = TYPE_C2_SOUTH_MARKER;			break;
	    case 8: dbp.type = TYPE_C2_START_BLIND_ZONE_FILTERING;	break;
	    case 9: dbp.type = TYPE_C2_STOP_BLIND_ZONE_FILTERING;	break;
	    default: dbp.type = NO_DETECTION;				break;
	}
	if (enviar) {
	    if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
	    }
        } else {
		update_calculations(dbp);
	}
    }
    return T_OK;
}

int ast_procesarCAT08(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
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
	    if (enviar) {
		if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		    exit(EXIT_FAILURE);
		}
	    } else {
		update_calculations(dbp);
	    }
	}
    }
    return T_OK;
}

int ast_procesarCAT10(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int size_current = 0, j = 0;
int index = 0;

    do {
	int sizeFSPEC;
	struct datablock_plot dbp;
	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);

	dbp.cat = CAT_10;
	dbp.available = IS_ERROR;
	dbp.type = NO_DETECTION;
	dbp.plot_type = IS_ERROR;
	dbp.modea_status = 0;
	dbp.modec_status = 0;
	dbp.flag_test = T_NO;
	dbp.flag_ground = T_NO;
	dbp.flag_sim = T_NO;
	dbp.tod_stamp = current_time; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;

	if (sizeFSPEC == 0) {
	    log_printf(LOG_WARNING, "ERROR_FSPEC_SIZE[%d] %s\n", sizeFSPEC, ptr_raw);
	    return T_ERROR;
	}
	
	j = sizeFSPEC; size_current += sizeFSPEC;
	if ( ptr_raw[0] & 128 ) { //I010/010
	    dbp.sac = ptr_raw[j];
	    dbp.sic = ptr_raw[j + 1];
	    j += 2; size_current += 2;
	    dbp.available |= IS_SACSIC;
	}
	
	if ( ptr_raw[0] & 64 ) { //I010/000
	    dbp.available |= IS_TYPE;
	    switch ( ptr_raw[j] ) {
		case 1 : dbp.type = TYPE_C10_TARGET_REPORT; break;
		case 2 : dbp.type = TYPE_C10_START_UPDATE_CYCLE; break;
		case 3 : dbp.type = TYPE_C10_PERIODIC_STATUS; break;
		case 4 : dbp.type = TYPE_C10_EVENT_STATUS; break;
		default: dbp.type = IS_ERROR; break;
	    }
	    j++; size_current++;
	}
	if ( ptr_raw[0] & 32 ) { //I010/020
	    int tmp = ptr_raw[j]>>5;
	    dbp.available |= IS_PLOT;
	    switch (tmp) {
		case 0: dbp.plot_type = TYPE_C10_PLOT_SSR_MULTI; break;
		case 1: dbp.plot_type = TYPE_C10_PLOT_SSRS_MULTI; break;
		case 2: dbp.plot_type = TYPE_C10_PLOT_ADSB; break;
		case 3: dbp.plot_type = TYPE_C10_PLOT_PSR; break;
		case 4: dbp.plot_type = TYPE_C10_PLOT_MAGNETIC; break;
		case 5: dbp.plot_type = TYPE_C10_PLOT_HF_MULTI; break;
		case 6: dbp.plot_type = TYPE_C10_PLOT_NOT_DEFINED; break;
		case 7: dbp.plot_type = TYPE_C10_PLOT_OTHER; break;
		default: dbp.plot_type = NO_DETECTION;
	    }
	    while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++;
	}
	if ( ptr_raw[0] & 16 ) { //I010/140
	   dbp.tod = ((float)(ptr_raw[j]<<16) +
	   (ptr_raw[j+1]<<8) +
	   (ptr_raw[j+2])) / 128.0;
	   j += 3; size_current += 3;
	    dbp.available |= IS_TOD;
	}
	if ( ptr_raw[0] & 8 ) { /* I010/041 */ j +=8; size_current += 8; }
	if ( ptr_raw[0] & 4 ) { /* I010/040 */ j += 4; size_current += 4;}
	if ( ptr_raw[0] & 2 ) { /* I010/042 */ j += 4; size_current += 4;}
	if ( ptr_raw[0] & 1 ) { /* FX1 */ 
	    if ( ptr_raw[1] & 128 ) { /* I010/200 */ j +=4; size_current += 4; }
	    if ( ptr_raw[1] & 64 ) { /* I010/202 */ j +=4; size_current += 4; }
	    if ( ptr_raw[1] & 32 ) { /* I010/161 */ j +=2; size_current += 2; }
	    if ( ptr_raw[1] & 16 ) { /* I010/170 */ while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
	    if ( ptr_raw[1] & 8 ) { /* I010/060 */ j +=2; size_current += 2; }
	    if ( ptr_raw[1] & 4 ) { /* I010/220 */ j +=3; size_current += 3; }
	    if ( ptr_raw[1] & 2 ) { /* I010/245 */ j +=7; size_current += 7; }
	    if ( ptr_raw[1] & 1 ) { /* FX2 */  
		if ( ptr_raw[2] & 128 ) { /* I010/250 */ size_current += ptr_raw[j] * 8 + 1; j += ptr_raw[j] * 8 + 1; }
		if ( ptr_raw[2] & 64 ) { /* I010/300 */ j++; size_current++; }
		if ( ptr_raw[2] & 32 ) { /* I010/090 */ j +=2; size_current += 2; }
		if ( ptr_raw[2] & 16 ) { /* I010/091 */ j +=2; size_current += 2; }
		if ( ptr_raw[2] & 8 ) { /* I010/270 */ while (ptr_raw[j] & 1) { j++; size_current++; }; j++; size_current++; }
		if ( ptr_raw[2] & 4 ) { /* I010/550 */ j++; size_current++; }
		if ( ptr_raw[2] & 2 ) { /* I010/310 */ j++; size_current++; }
		if ( ptr_raw[2] & 1 ) { /* FX3 */
		    if ( ptr_raw[3] & 128 ) { /* I010/500 */ j += 4; size_current += 4; }
		    if ( ptr_raw[3] & 64 ) { /* I010/280 */ size_current += ptr_raw[j] * 2 + 1; j += ptr_raw[j] * 2 + 1; }
		    if ( ptr_raw[3] & 32 ) { /* I010/131 */ j++; size_current++; }
		    if ( ptr_raw[3] & 16 ) { /* I010/210 */ j +=2; size_current += 2; }
		    if ( ptr_raw[3] & 8 ) { /* SPARE */ }
		    if ( ptr_raw[3] & 4 ) { /* SP */ while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
		    if ( ptr_raw[3] & 2 ) { /* RE */ while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
		}
	    }
	}
//	ast_output_datablock(ptr_raw, j , dbp.id, dbp.index);
//	if ( (dbp.available & IS_TYPE) && (dbp.available & IS_TOD) ) {
	if ( dbp.available != IS_ERROR ) {
/*	    log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
	    if ((dbp.tod_stamp - dbp.tod < 0) || ((dbp.tod_stamp - dbp.tod > 5)) ) {
		log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);	
		exit(EXIT_FAILURE);
	    }
*/
	    if (enviar) {
		if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		}
	    } else {
		update_calculations(dbp);
	    }
	}
	
	ptr_raw += j;
	index++;
    } while ((size_current + 3) < size_datablock);

    return T_OK;
}

int ast_procesarCAT19(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int sizeFSPEC=0, pos=0;
struct datablock_plot dbp;

    dbp.plot_type = IS_ERROR;
    dbp.tod_stamp = current_time;
    dbp.flag_test = 0;
    dbp.id = id;
    dbp.index = 0;
    sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
//    ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);

    if ( (ptr_raw[0] & 128) && // sac/sic
	 (ptr_raw[0] & 64) &&  // msg type
	 (ptr_raw[0] & 32) ) { // timeofday
	
	pos += sizeFSPEC + 3; //FSPEC SACSIC MSGTYPE
	ttod_put_full(ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC + 1], ptr_raw + pos);

	dbp.cat = CAT_19;
	dbp.sac = ptr_raw[sizeFSPEC]; dbp.sic = ptr_raw[sizeFSPEC+1];
	dbp.available = IS_TOD | IS_TYPE | IS_SACSIC;
	dbp.tod = ((float)( (ptr_raw[pos]<<16) + (ptr_raw[pos+1]<<8) + ptr_raw[pos+2]))/128.0;
	switch(ptr_raw[sizeFSPEC+2]) {
	    case 1: dbp.type = TYPE_C19_START_UPDATE_CYCLE;		break;
	    case 2: dbp.type = TYPE_C19_PERIODIC_STATUS;		break;
	    case 3: dbp.type = TYPE_C19_EVENT_STATUS;			break;
	    default: dbp.type = IS_ERROR;				break;
	}
	if (enviar) {
	    if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
	    }
        } else {
	    update_calculations(dbp);
	}
    }
    return T_OK;
}

int ast_procesarCAT20(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int size_current = 0, j = 0;
int index = 0;

    do {
	int sizeFSPEC;
	struct datablock_plot dbp;
	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
/*	{ 
	    int i=0; 
	    for(i=0;i<sizeFSPEC;i++) {
		log_printf(LOG_VERBOSE, "%02X ", ptr_raw[i]);
	    }
	    log_printf(LOG_VERBOSE, "\n");
	}*/

	dbp.cat = CAT_20;
	dbp.available = IS_ERROR;
	dbp.type = NO_DETECTION;
	dbp.plot_type = IS_ERROR;
	dbp.modea_status = 0;
	dbp.modec_status = 0;
	dbp.flag_test = T_NO;
	dbp.flag_ground = T_NO;
	dbp.flag_sim = T_NO;
	dbp.tod_stamp = current_time; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;

//	ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);
	if (sizeFSPEC == 0) {
	    log_printf(LOG_WARNING, "ERROR_FSPEC_SIZE[%d] %s\n", sizeFSPEC, ptr_raw);
	    return T_ERROR;
	}
	
	j = sizeFSPEC; size_current += sizeFSPEC;
	if ( ptr_raw[0] & 128 ) { //I020/010
	    dbp.sac = ptr_raw[j];
	    dbp.sic = ptr_raw[j + 1];
	    j += 2; size_current += 2;
	    dbp.available |= IS_SACSIC;
	}
	if ( ptr_raw[0] & 64 ) { /* I020/020 */  // FIXME
	    dbp.type = 0;
	    while (ptr_raw[j] & 1) { 
		dbp.type = dbp.type<<8; dbp.type += ptr_raw[j]; 
		j++; size_current++; 
	    }; 
	    dbp.type = dbp.type<<8; dbp.type += ptr_raw[j]; 
	    j++;size_current++; 
	    dbp.available |= IS_TYPE;
	}
	if ( ptr_raw[0] & 32 ) { //I020/140
	   dbp.tod = ((float)(ptr_raw[j]<<16) +
	   (ptr_raw[j+1]<<8) +
	   (ptr_raw[j+2])) / 128.0;
	   j += 3; size_current += 3;
	    dbp.available |= IS_TOD;
	}
	if ( ptr_raw[0] & 16 ) { /* I020/041 */ j += 8; size_current += 8; }
	if ( ptr_raw[0] & 8 ) { /* I020/042 */ j += 6; size_current += 6; }
	if ( ptr_raw[0] & 4 ) { /* I020/161 */ j += 2; size_current += 2; }
	if ( ptr_raw[0] & 2 ) { /* I020/170 */ while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
	if ( ptr_raw[0] & 1 ) { /* I020/FX1 */
	    if ( ptr_raw[1] & 128 ) { /* I010/070 */ j += 2; size_current += 2; }
	    if ( ptr_raw[1] & 64 ) { /* I010/202 */ j += 4; size_current += 4; }
	    if ( ptr_raw[1] & 32 ) { /* I010/090 */ j += 2; size_current += 2; }
	    if ( ptr_raw[1] & 16 ) { /* I020/100 */ j += 4; size_current += 4; }
	    if ( ptr_raw[1] & 8 ) { /* I020/220 */ j += 3; size_current += 3; }
	    if ( ptr_raw[1] & 4 ) { /* I020/245 */ j += 7; size_current += 7; }
	    if ( ptr_raw[1] & 2 ) { /* I020/110 */ j += 2; size_current += 2; }
	    if ( ptr_raw[1] & 1 ) { /* I020/FX2 */
		if ( ptr_raw[2] & 128 ) { /* I020/105 */ j += 2; size_current += 2; }
		if ( ptr_raw[2] & 64 ) { /* I020/210 */ j += 2; size_current += 2; }
		if ( ptr_raw[2] & 32 ) { /* I020/300 */ j += 1; size_current += 1; }
		if ( ptr_raw[2] & 16 ) { /* I020/310 */ j += 1; size_current += 1; }
		if ( ptr_raw[2] & 8 ) { /* I020/500 */ 
		    int p = j;
		    if (ptr_raw[p] & 128) { j += 6; size_current += 6; }
		    if (ptr_raw[p] & 64) { j += 6; size_current += 6; }
		    if (ptr_raw[p] & 32) { j += 2; size_current += 2; }
		    j++; size_current++;
		}
		if ( ptr_raw[2] & 4 ) { /* I020/400 */ int p = ptr_raw[j]+1; j += p; size_current += p; }
		if ( ptr_raw[2] & 2 ) { /* I020/250 */ int p = ptr_raw[j]*8+1; j += p; size_current += p; }
		if ( ptr_raw[2] & 1 ) { /* I020/FX3 */ 
		    if ( ptr_raw[3] & 128 ) { /* I020/230 */ j += 2; size_current += 2; }
		    if ( ptr_raw[3] & 64 ) { /* I020/260 */ j += 7; size_current += 7; }
		    if ( ptr_raw[3] & 32 ) { /* I020/030 */ while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
		    if ( ptr_raw[3] & 16 ) { /* I020/055 */ j++; size_current++; }
		    if ( ptr_raw[3] & 8 ) { /* I020/050 */ j += 2; size_current += 2; }
		    if ( ptr_raw[3] & 4 ) { /* RE */ int p = ptr_raw[j]; j += p; size_current += p; }
		}
	    }
	}
	
//	ast_output_datablock(ptr_raw, j , dbp.id, dbp.index);
//	if ( (dbp.available & IS_TYPE) && (dbp.available & IS_TOD) ) {
//	if ( dbp.available != IS_ERROR ) {
//	    log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
/*	    if ((dbp.tod_stamp - dbp.tod < 0) || ((dbp.tod_stamp - dbp.tod > 5)) ) {
		log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);	
		exit(EXIT_FAILURE);
//	    }
*/	    if (enviar) {
		if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		}
	    } else {
		update_calculations(dbp);
	    }
//	}
	
	ptr_raw += j;
	index++;
    } while ((size_current + 3) < size_datablock);

    return T_OK;
}



int ast_procesarCAT21(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int size_current = 0, j = 0;
int index = 0;

    do {
	int sizeFSPEC;
	struct datablock_plot dbp;
	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);

	dbp.cat = CAT_21;
	dbp.available = IS_ERROR;
	dbp.type = NO_DETECTION;
	dbp.plot_type = IS_ERROR;
	dbp.modea_status = 0;
	dbp.modec_status = 0;
	dbp.flag_test = T_NO;
	dbp.flag_ground = T_NO;
	dbp.flag_sim = T_NO;
	dbp.flag_fixed = T_NO;
	dbp.tod_stamp = current_time; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;

	if (sizeFSPEC == 0) {
	    log_printf(LOG_WARNING, "ERROR_FSPEC_SIZE[%d] %s\n", sizeFSPEC, ptr_raw);
	    return T_ERROR;
	}
	
	j = sizeFSPEC; size_current += sizeFSPEC;
	if ( ptr_raw[0] & 128 ) { //I021/010
	    dbp.sac = ptr_raw[j];
	    dbp.sic = ptr_raw[j + 1];
	    j += 2; size_current += 2;
	    dbp.available |= IS_SACSIC;
	}
	
	if ( ptr_raw[0] & 64 ) { //I021/040
	    dbp.available |= IS_TYPE;
	    if ( (ptr_raw[j] & 64) == 64 ) dbp.flag_ground = T_YES;
	    if ( (ptr_raw[j] & 32) == 32 ) dbp.flag_sim = T_YES;
	    if ( (ptr_raw[j] & 16) == 16 ) dbp.flag_test = T_YES;
	    if ( (ptr_raw[j] & 8) == 8 ) dbp.flag_fixed = T_YES;
	    j+=2; size_current+=2;
	}
	if ( ptr_raw[0] & 32 ) { //I021/030
	   dbp.tod = ((float)(ptr_raw[j]<<16) +
	   (ptr_raw[j+1]<<8) +
	   (ptr_raw[j+2])) / 128.0;
	   j += 3; size_current += 3;
	    dbp.available |= IS_TOD;
	}
	if ( ptr_raw[0] & 16 ) { /* I021/130 */ j +=8; size_current += 8; }
	if ( ptr_raw[0] & 8 ) { /* I021/080 */ j +=3; size_current += 3; }
	if ( ptr_raw[0] & 4 ) { /* I021/140 */ j += 2; size_current += 2;}
	if ( ptr_raw[0] & 2 ) { /* I021/090 */ j += 2; size_current += 2;}
	if ( ptr_raw[0] & 1 ) { /* FX1 */ 
	    if ( ptr_raw[1] & 128 ) { /* I021/210 */ j++; size_current++; }
	    if ( ptr_raw[1] & 64 ) { /* I021/230 */ j +=2; size_current += 2; }
	    if ( ptr_raw[1] & 32 ) { /* I021/145 */ j +=2; size_current += 2; }
	    if ( ptr_raw[1] & 16 ) { /* I021/150 */ j +=2; size_current += 2; } //while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
	    if ( ptr_raw[1] & 8 ) { /* I021/151 */ j +=2; size_current += 2; }
	    if ( ptr_raw[1] & 4 ) { /* I021/152 */ j +=2; size_current += 2; }
	    if ( ptr_raw[1] & 2 ) { /* I021/155 */ j +=2; size_current += 2; }
	    if ( ptr_raw[1] & 1 ) { /* FX2 */
		if ( ptr_raw[2] & 128 ) { /* I021/157 */ j +=2; size_current += 2; } //size_current += ptr_raw[j] * 8 + 1; j += ptr_raw[j] * 8 + 1; }
		if ( ptr_raw[2] & 64 ) { /* I021/160 */ j +=4; size_current += 4; }
		if ( ptr_raw[2] & 32 ) { /* I021/165 */ while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
		if ( ptr_raw[2] & 16 ) { /* I021/170 */ j +=6; size_current += 6; }
		if ( ptr_raw[2] & 8 ) { /* I021/095 */ j++; size_current++; }
		if ( ptr_raw[2] & 4 ) { /* I021/032 */ j++; size_current++; }
		if ( ptr_raw[2] & 2 ) { /* I021/200 */ j++; size_current++; }
		if ( ptr_raw[2] & 1 ) { /* FX3 */
		    if ( ptr_raw[3] & 128 ) { /* I021/020 */ j++; size_current++; }
		    if ( ptr_raw[3] & 64 ) { /* I021/220 */ 
			int add=0;
			if ( (ptr_raw[j] & 128) == 128 ) add += 2;
			if ( (ptr_raw[j] & 64) == 64 ) add += 2;
			if ( (ptr_raw[j] & 32) == 32 ) add += 2;
			if ( (ptr_raw[j] & 16) == 16 ) add++;
			j += add; size_current += add;
		    }
		    if ( ptr_raw[3] & 32 ) { /* I021/146 */ j += 2; size_current += 2; }
		    if ( ptr_raw[3] & 16 ) { /* I021/148 */ j += 2; size_current += 2; }
		    if ( ptr_raw[3] & 8 ) { /* I021/110 */
			int add = 0;
			if ( (ptr_raw[j] & 128) == 128 ) add += 2;
			if ( (ptr_raw[j] & 64) == 64 ) { 
			    add += ptr_raw[j + add]*15 + 1;
			}
			j += add; size_current += add;
		     }
		    if ( ptr_raw[3] & 4 ) { /* I021/070 */ j += 2; size_current += 2; }
		    if ( ptr_raw[3] & 2 ) { /* I021/131 */ j++; size_current++; }
		    if ( ptr_raw[3] & 1 ) { /* FX4 */
			if ( ptr_raw[4] & 4 ) { while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
			if ( ptr_raw[4] & 2 ) { while (ptr_raw[j] & 1) { j++; size_current++; }; j++;size_current++; }
		    }
		}
	    }
	}
//	ast_output_datablock(ptr_raw, j , dbp.id, dbp.index);
//	if ( (dbp.available & IS_TYPE) && (dbp.available & IS_TOD) ) {
	if ( dbp.available != IS_ERROR ) {
/*	    log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
	    if ((dbp.tod_stamp - dbp.tod < 0) || ((dbp.tod_stamp - dbp.tod > 5)) ) {
		log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);	
		exit(EXIT_FAILURE);
	    }
*/
	    if (enviar) {
		if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		}
	    } else {
		update_calculations(dbp);
	    }
	}
	
	ptr_raw += j;
	index++;
    } while ((size_current + 3) < size_datablock);

    return T_OK;
}

int ast_procesarCAT34(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int size_current = 0, j = 0;
int index = 0;

    do {
	int sizeFSPEC;
	struct datablock_plot dbp;
	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);

	dbp.cat = CAT_34;
	dbp.available = IS_ERROR;
	dbp.type = NO_DETECTION;
	dbp.plot_type = IS_ERROR;
	dbp.modea_status = 0;
	dbp.modec_status = 0;
	dbp.flag_test = T_NO;
	dbp.flag_ground = T_NO;
	dbp.flag_sim = T_NO;
	dbp.tod_stamp = current_time; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;

	//ast_output_datablock(ptr_raw, size_datablock, dbp.id, dbp.index);
	if (sizeFSPEC == 0) {
	    log_printf(LOG_WARNING, "ERROR_FSPEC_SIZE[%d] %s\n", sizeFSPEC, ptr_raw);
	    return T_ERROR;
	}
	
	j = sizeFSPEC; size_current += sizeFSPEC;

	if ( (ptr_raw[0] & 128) && // sac/sic
	     (ptr_raw[0] & 64) &&  // msg type
	    (ptr_raw[0] & 32) ) { // timeofday
	
	    dbp.cat = CAT_34;
	    dbp.sac = ptr_raw[j]; dbp.sic = ptr_raw[j+1];

	    switch(ptr_raw[j+2]) {
		case 1: dbp.type = TYPE_C34_NORTH_MARKER;		break;
		case 2: dbp.type = TYPE_C34_SECTOR_CROSSING;		break;
		case 3: dbp.type = TYPE_C34_GEOGRAPHICAL_FILTERING;	break;
		case 4: dbp.type = TYPE_C34_JAMMING_STROBE;		break;
		default: dbp.type = NO_DETECTION;				break;
	    }

	    dbp.available = IS_TOD | IS_TYPE | IS_SACSIC;
	    size_current += 3; j += 3;
	    dbp.tod = ((float)(ptr_raw[j]*256*256 + ptr_raw[j+1]*256 + ptr_raw[j+2]))/128.0;
	    size_current += 3; j += 3;
	    if (enviar) {
		if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		}
	    } else {
	        update_calculations(dbp);
	    }
	    ptr_raw += j;
	    index++;
	}
	
	// FIXME // el resto de dataitems los ignoramos	
	size_current = size_datablock - 3;

    } while ((size_current + 3) < size_datablock);

    return T_OK;
}

int ast_procesarCAT48(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int size_current = 0, j = 0;
int index = 0;

    do {
	int sizeFSPEC;
	struct datablock_plot dbp;

//	log_printf(LOG_NORMAL, "fspec %02X\n", ptr_raw[0]);

	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
	dbp.cat = CAT_48;
	dbp.available = IS_ERROR;
	dbp.type = NO_DETECTION;
	dbp.plot_type = IS_ERROR;
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
	if ( ptr_raw[0] & 128 ) { //I048/010
	    dbp.sac = ptr_raw[sizeFSPEC];
	    dbp.sic = ptr_raw[sizeFSPEC + 1];
	    j += 2; size_current += 2;
	    dbp.available |= IS_SACSIC;
	}
	if ( ptr_raw[0] & 64  ) { //I048/140
	    //log_printf(LOG_NORMAL, "tod %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
	    dbp.tod = ((float)(ptr_raw[j]*256*256 + ptr_raw[j+1]*256 + ptr_raw[j+2]))/128.0;
	    size_current += 3; j += 3;
	    dbp.available |= IS_TOD;
	}

	if ( dbp.available & IS_TOD ) {
/*	    log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
	    if ((dbp.tod_stamp - dbp.tod < 0) || ((dbp.tod_stamp - dbp.tod > 5)) ) {
		log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);	
		exit(EXIT_FAILURE);
	    }
*/	    if (enviar) {
		if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		}
	    } else {
		update_calculations(dbp);
	    }
	}
	
	// FIXME
	
        ptr_raw += j;
	index++;
	size_current = size_datablock - 3;
	
    } while ((size_current + 3) < size_datablock);

    return T_OK;
}


int ast_procesarCAT62(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
/*int sizeFSPEC=0;
struct datablock_plot dbp;

    dbp.tod_stamp = current_time; dbp.id = id; dbp.index = 0;
    sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
//    ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);
*/    return T_OK;
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

//    log_printf(LOG_ERROR, "d)CAT02] (-) %02X %02X\n", sac, sic);
//    log_printf(LOG_ERROR, "c)CAT02] (%d) %02X %02X \n", sizeFSPEC, ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC+1] );
//    log_printf(LOG_ERROR, "e)CAT02] (-) %02X %02X array[%02X%02X] i(%d)\n", sac, sic, full_tod[i], full_tod[i+1],i);

/*    while ( (i < (MAX_RADAR_NUMBER*TTOD_WIDTH)) &&
	    (full_tod[i] != 0) &&
	    (full_tod[i+1] != 0) &&
	    (full_tod[i] != sac) && 
	    (full_tod[i+1] != sic) ) {
	        log_printf(LOG_ERROR, "f)CAT02] (-) %02X %02X array[%02X%02X] i(%d)\n", sac, sic, full_tod[i], full_tod[i+1],i);
		i+=TTOD_WIDTH;
	    }
*/
    for(i=0;i<MAX_RADAR_NUMBER*TTOD_WIDTH;i+=TTOD_WIDTH) {
	if ( (full_tod[i] == 0) && (full_tod[i+1] == 0) )
	    break;
	if ( (full_tod[i] == sac) && (full_tod[i+1] == sic) )
	    break;
//	log_printf(LOG_ERROR, "f)CAT02] (-) %02X %02X array[%02X%02X] i(%d)\n", sac, sic, full_tod[i], full_tod[i+1],i);
    }


//    log_printf(LOG_ERROR, "g)CAT02] (-) %02X %02X array[%02X%02X] i(%d)\n", sac, sic, full_tod[i], full_tod[i+1],i);

    if ( i < MAX_RADAR_NUMBER*TTOD_WIDTH ) {
	if ((full_tod[i] == 0) && (full_tod[i+1] == 0)) {
	    full_tod[i] = sac;
	    full_tod[i+1] = sic;
	}
//	if (full_tod[i]!=sac || full_tod[i+1]!=sic) {
//	    log_printf(LOG_ERROR, "ERROR\n");
//	}
//	log_printf(LOG_ERROR, "h)CAT02] (-) %02X %02X\n", sac, sic);

//	if ((ptr_full_tod[0] == 0) && (ptr_full_tod[1] == 0) && (ptr_full_tod[2] == 0)) {
//	}

	full_tod[i+2] = ptr_full_tod[0];
	full_tod[i+3] = ptr_full_tod[1];
	full_tod[i+4] = ptr_full_tod[2];
//	if (sac==0x20 && sic==0x1)
//	log_printf(LOG_ERROR, "i)CAT02] (-) %02X %02X\n", sac, sic);
//      log_printf(LOG_ERROR, ">NRT  %02X%02X%02X (%02X %02X)\n", full_tod[i+2], full_tod[i+3], full_tod[i+4], full_tod[i], full_tod[i+1]);
	full_tod[i+5] = ptr_full_tod[1];
	full_tod[i+6] = ptr_full_tod[2];
    }
    return;
}

float ttod_get_full(int sac, int sic, unsigned char *ptr_ttod, unsigned long index) {
int i = 0;
int adjust = 0;

//  while ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) &&
//	    (full_tod[i] != 0) &&
//	    (full_tod[i+1] != 0) &&
//	    (full_tod[i] != (unsigned char) sac) && 
//	    (full_tod[i+1] != (unsigned char) sic) )
//	    i+=TTOD_WIDTH;

    for(i=0;i<MAX_RADAR_NUMBER*TTOD_WIDTH;i+=TTOD_WIDTH) {
	if ( (full_tod[i] == 0) && (full_tod[i+1] == 0) )
    	    break;
        if ( (full_tod[i] == sac) && (full_tod[i+1] == sic) )
    	    break;
	//log_printf(LOG_ERROR, "f)CAT02] (-) %02X %02X array[%02X%02X] i(%d)\n", sac, sic, full_tod[i], full_tod[i+1],i);
    }
                                              


    if ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) && (full_tod[i] != 0) && (full_tod[i+1] != 0) ) {
	long p = (full_tod[i+5]<<8) + full_tod[i+6];
	long q = (ptr_ttod[0]<<8) + ptr_ttod[1];
	long d = abs(q-p);

	//printTOD();		
	//log_printf(LOG_VERBOSE, "SAC: %d SIC: %d\n", sac, sic);
	//log_printf(LOG_VERBOSE, "\t>NRT %02X%02X%02X %d\n", full_tod[i+2], full_tod[i+3], full_tod[i+4],
	//	(full_tod[i+2]<<16) + (full_tod[i+3]<<8) + (full_tod[i+4]) );
	//log_printf(LOG_VERBOSE, "\tPLT    %02X%02X p:%ld q:%ld q-p:%d\n", ptr_ttod[0], ptr_ttod[1], p, q, abs(q-p) );
	
	if ( d <= 16*128*5 ) {		// dentro de la ventana de seguimiento (16 segundos, 2 vueltas antena)
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
        } else { //if ( (abs(q-p)>16*128) && (abs(q-p)<=60000) ) { // ERROR, fuera de ventanas de seguimiento
	    //	SAC: 20 SIC: 133
	    //	NRT  2A1C0F 2759695
	    //	PLT    FFC3 p:7240 q:65475 q-p:58235
	    //	ADJ5 2AFFC3 2817987/128.0 ERROR
	    //	Sun Jul  8 05:59:20 2007
	    //printTOD();
	    time_t secs = floor((full_tod[i+2]<<16) + (full_tod[i+3]<<8) + (full_tod[i+4])/128.0);
	    struct tm *tm_s;
	    char str[256];
	    tm_s = localtime(&secs); 
	    strftime(str, 256, "%H:%M:%S", tm_s);
	    
	    log_printf(LOG_ERROR, "index(%ld) sac(%d) sic(%d)\n", index,sac, sic);
	    log_printf(LOG_ERROR, "NRT  [%02X%02X%02X] [%d->%s.%03.0f]\n", full_tod[i+2], full_tod[i+3], full_tod[i+4],
		(full_tod[i+2]<<16) + (full_tod[i+3]<<8) + (full_tod[i+4]), str, (((full_tod[i+2]<<16) + (full_tod[i+3]<<8) + (full_tod[i+4])/128.0) - secs)*1000);
	    log_printf(LOG_ERROR, "PLT    [%02X%02X] p:%ld q:%ld q-p:%d\n", ptr_ttod[0], ptr_ttod[1], p, q, abs(q-p) );

	    secs = floor(((full_tod[i+2]<<16) + q)/128.0);
	    tm_s = localtime(&secs); 
	    strftime(str, 256, "%H:%M:%S", tm_s);

	    log_printf(LOG_ERROR, "ADJ5 [%02X%02X%02X] [%ld->%s.%03.0f] ERROR\n", full_tod[i+2], ptr_ttod[0], ptr_ttod[1], 
		((full_tod[i+2]<<16) + q) , str, (((full_tod[i+2]<<16) + q)/128.0 - secs)*1000);
	    
	    log_printf(LOG_ERROR, "-------------------------------------------------------------\n");
	    return T_ERROR;
	}
	return ((float)( ((full_tod[i+2] + adjust)<<16) + q))/128.0;
    }
    return T_ERROR;
};

#ifndef CLIENT_RRD
void update_calculations(struct datablock_plot dbp) {
    return;
}    
#endif
