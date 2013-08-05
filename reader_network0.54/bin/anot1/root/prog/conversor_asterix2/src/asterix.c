               :
               :#include "includes.h"
               :
               :extern float current_time;
               :extern int s;
               :extern struct sockaddr_in srvaddr;
               :
               :void ast_output_datablock(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, unsigned long index) {
               :int i;
               :unsigned char *ptr_tmp;
               :
    30  0.3714 :    ptr_tmp = (unsigned char *) mem_alloc(size_datablock*3 + 2);
    21  0.2600 :    bzero(ptr_tmp, size_datablock*3 + 2);
               :    
  2594 32.1159 :    for (i=0; i < size_datablock*3; i+=3)
  2419 29.9492 :	sprintf(ptr_tmp + i, "%02X ", ptr_raw[i/3]);
    62  0.7676 :    log_printf(LOG_VERBOSE, "%ld.%0ld %s\n", id, index, ptr_tmp);
               :
   126  1.5600 :    mem_free(ptr_tmp);
               :    return;
    14  0.1733 :}
               :
               :int ast_get_size_FSPEC(unsigned char *ptr_raw, ssize_t size_datablock) {
     1  0.0124 :int sizeFSPEC = 0;
               :
               :    //no distincion entre plot y track
     7  0.0867 :    if ( size_datablock <= 3 ) return -1;
               :	
    42  0.5200 :    if ( ptr_raw[0] & 1 ) {
               :	if ( size_datablock < 5) { return -1; }
     1  0.0124 :	if ( ptr_raw[1] & 1 ) {
               :    	    if ( size_datablock < 6) { return -1; }
               :	    if ( ptr_raw[2] & 1 ) {
               :		if ( size_datablock < 7 ) { return -1; }
               :		sizeFSPEC = 4;
               :    	    } else {
               :		sizeFSPEC = 3;
               :    	    }
               :        } else {
               :    	    sizeFSPEC = 2;
               :	}
     1  0.0124 :    } else  {
     1  0.0124 :	sizeFSPEC = 1;
               :    }
               :    
    21  0.2600 :    return sizeFSPEC;
    18  0.2229 :}
               :
               :int ast_procesarCAT01(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id) {
               :int size_current = 0, j = 0;
               :int index = 0;
               :    do {
               :	int sizeFSPEC;
               :	struct datablock_plot dbp;
               :
   112  1.3867 :	log_printf(LOG_NORMAL, "fspec %02X\n", ptr_raw[0]);
               :
    58  0.7181 :	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
     6  0.0743 :	dbp.cat = CAT_01;
     2  0.0248 :	dbp.available = IS_ERROR;
               :	dbp.type = NO_DETECTION;
     8  0.0990 :        dbp.modea_status = 0;
     1  0.0124 :        dbp.modec_status = 0;
   119  1.4733 :	dbp.tod_stamp = current_time; dbp.id = id; dbp.index = index;
               :    
     5  0.0619 :	if (sizeFSPEC == 0) {
               :	    log_printf(LOG_WARNING, "ERROR_FSPEC_SIZE[%d] %s\n", sizeFSPEC, ptr_raw);
               :	    return T_ERROR;
               :	}
               :	
    31  0.3838 :	j = sizeFSPEC;
               :	size_current += sizeFSPEC;
     7  0.0867 :	if ( ptr_raw[0] & 128 ) { //I001/010
     4  0.0495 :	    dbp.sac = ptr_raw[sizeFSPEC];
     4  0.0495 :	    dbp.sic = ptr_raw[sizeFSPEC + 1];
     3  0.0371 :	    j += 2; size_current += 2;
     6  0.0743 :	    dbp.available |= IS_SACSIC;
               :	}
     6  0.0743 :	if ( ptr_raw[0] & 64 ) { //I001/010
    10  0.1238 :	    if ( ptr_raw[j] & 128 ) { // track
               :	        dbp.available |= IS_TRACK;
               :		size_current = size_datablock - 3; //exit without further decompression
               :	    
               :	    } else { // plot
     5  0.0619 :		dbp.available |= IS_TYPE;
    43  0.5324 :		log_printf(LOG_NORMAL, "type %02X\n", ptr_raw[j]);
               :
    66  0.8171 :		if ( (!(ptr_raw[j] & 32)) && (!(ptr_raw[j] & 16)) ) {
               :		    dbp.type = NO_DETECTION;
    28  0.3467 :		} else if ( (!(ptr_raw[j] & 32)) && (ptr_raw[j] & 16) ) {
               :		    dbp.type = TYPE_C1_PSR;
    28  0.3467 :		} else if ( (ptr_raw[j] & 32) && (!(ptr_raw[j] & 16)) ) {
     1  0.0124 :		    dbp.type = TYPE_C1_SSR;
     7  0.0867 :		} else if ( (ptr_raw[j] & 32) && (ptr_raw[j] & 16) ) {
     1  0.0124 :		    dbp.type = TYPE_C1_CMB;
               :		}
    14  0.1733 :		if ( ptr_raw[j] & 2 ) {
               :		    dbp.type |= FROM_C1_FIXED_TRANSPONDER;
               :		}
    31  0.3838 :		while (ptr_raw[j] & 1) { j++; size_current++;;}
     2  0.0248 :		size_current++; j++;
     8  0.0990 :		if ( ptr_raw[0] & 32 ) { //I001/040
    35  0.4333 :		    log_printf(LOG_NORMAL, "polar %02X %02X %02X %02X \n", ptr_raw[j], ptr_raw[j+1], ptr_raw[j+2], ptr_raw[j+3] );
    86  1.0648 :	    	    dbp.rho = (ptr_raw[j]*256 + ptr_raw[j+1]) / 128.0;
    26  0.3219 :	    	    dbp.theta = (ptr_raw[j+2]*256 + ptr_raw[j+3]) * 360.0/65536.0;
     8  0.0990 :	    	    size_current += 4; j+= 4;
     6  0.0743 :	    	    dbp.available |= IS_MEASURED_POLAR;
               :	        }
     7  0.0867 :		if ( ptr_raw[0] & 16 ) { //I001/070
    35  0.4333 :		    log_printf(LOG_NORMAL, "modea %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
    65  0.8048 :		    dbp.modea_status |= (ptr_raw[j] & 128) ? STATUS_MODEA_NOTVALIDATED : 0;
    24  0.2971 :		    dbp.modea_status |= (ptr_raw[j] & 64) ? STATUS_MODEA_GARBLED : 0;
    14  0.1733 :		    dbp.modea_status |= (ptr_raw[j] & 32) ? STATUS_MODEA_SMOOTHED : 0;
    29  0.3590 :		    dbp.modea = (ptr_raw[j] & 15)*256 + ptr_raw[j+1];
     6  0.0743 :		    size_current += 2; j += 2;
     6  0.0743 :		    dbp.available |= IS_MODEA;
               :		}
     3  0.0371 :		if ( ptr_raw[0] & 8  ) { //I001/090
    50  0.6190 :		    log_printf(LOG_NORMAL, "modec %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
    35  0.4333 :		    dbp.modec_status |= (ptr_raw[j] & 128) ? STATUS_MODEC_NOTVALIDATED : 0;
    22  0.2724 :		    dbp.modec_status |= (ptr_raw[j] & 64) ? STATUS_MODEC_GARBLED : 0;
    64  0.7924 :		    dbp.modec = ( (ptr_raw[j] & 63)*256 + ptr_raw[j+1]) * 1.0/4.0;// * 100;
    20  0.2476 :		    size_current += 2; j += 2;
     2  0.0248 :		    dbp.available |= IS_MODEC;
               :		}
    23  0.2848 :		if ( ptr_raw[0] & 4  ) { //I001/130
    23  0.2848 :		    log_printf(LOG_NORMAL, "responses %02X\n", ptr_raw[j]);
    33  0.4086 :	    	    dbp.radar_responses = (ptr_raw[j] >> 1);
    17  0.2105 :		    while (ptr_raw[j] & 1) { j++; size_current++; }
    16  0.1981 :		    size_current++; j++;
               :		    dbp.available |= IS_RADAR_RESPONSES;
               :		}
     6  0.0743 :		if ( ptr_raw[0] & 2  ) { //I001/141
    21  0.2600 :		    log_printf(LOG_NORMAL, "tod %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
    94  1.1638 :	    	    dbp.truncated_tod = (ptr_raw[j]*256 + ptr_raw[j+1]) / 128.0;
    27  0.3343 :		    if ( (dbp.tod = ttod_get_full(dbp.sac, dbp.sic, ptr_raw + j)) != T_ERROR )
               :			dbp.available |= IS_TOD;
     3  0.0371 :		    size_current += 2; j += 2;
     6  0.0743 :		    dbp.available |= IS_TRUNCATED_TOD;
               :		}
     2  0.0248 :		if ( (sizeFSPEC > 1) && (ptr_raw[1] & 16) ) { //I001/080
               :		    size_current += 2; j += 2;
               :		}
    20  0.2476 :		if ( (sizeFSPEC > 1) && (ptr_raw[1] & 8) ) { //I001/100
               :		    size_current += 4; j += 4;
               :		}
               :	    }
               :	}
    32  0.3962 :	ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);
    61  0.7552 :	if ( (dbp.available & IS_TYPE) && (dbp.available & IS_TOD) ) {
               ://	    log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
               :/*	    if (dbp.tod_stamp - dbp.tod < 0) {
               :		log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);	
               :		exit(EXIT_FAILURE);
               :	    }
   133  1.6467 :*/	    if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
               :		log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
               :	    }
               :	}
               :	
    15  0.1857 :        ptr_raw += j;
     2  0.0248 :	index++;
     5  0.0619 :    } while ((size_current + 3) < size_datablock);
               :
     1  0.0124 :    return T_OK;
     3  0.0371 :}
               :
               :int ast_procesarCAT02(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id) {
               :int sizeFSPEC=0, pos=0;
               :struct datablock_plot dbp;
               :
               :    dbp.tod_stamp = current_time;
               :    dbp.id = id;
               :    dbp.index = 0;
               :    sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
               :
     3  0.0371 :    if ( (ptr_raw[0] & 128) && // sac/sic
               :	 (ptr_raw[0] & 64) &&  // msg type
               :	 (ptr_raw[0] & 16) ) { // timeofday
               :	
               :	if (ptr_raw[0] & 32) pos++;
               :	
     3  0.0371 :	pos += sizeFSPEC + 3; //FSPEC SACSIC MSGTYPE
     3  0.0371 :	ttod_put_full(ptr_raw[sizeFSPEC], ptr_raw[sizeFSPEC + 1], ptr_raw + pos);
               :
               :	dbp.cat = CAT_02;
               :	dbp.sac = ptr_raw[sizeFSPEC]; dbp.sic = ptr_raw[sizeFSPEC+1];
               :	dbp.available = IS_TOD | IS_TYPE | IS_SACSIC;
     3  0.0371 :	dbp.tod = ((float)(ptr_raw[pos]*256*256 + ptr_raw[pos+1]*256 + ptr_raw[pos+2]))/128.0;
               :	switch(ptr_raw[sizeFSPEC+2]) {
               :	    case 1: dbp.type = TYPE_C2_NORTH_MARKER;			break;
               :	    case 2: dbp.type = TYPE_C2_SECTOR_CROSSING;			break;
               :	    case 3: dbp.type = TYPE_C2_SOUTH_MARKER;			break;
               :	    case 8: dbp.type = TYPE_C2_START_BLIND_ZONE_FILTERING;	break;
               :	    case 9: dbp.type = TYPE_C2_STOP_BLIND_ZONE_FILTERING;	break;
               :	    default: dbp.type = NO_DETECTION;				break;
               :	}
     2  0.0248 :	ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);
     3  0.0371 :	if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
               :	    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
               :	}
               :    }
     1  0.0124 :    return T_OK;
               :}
               :
               :int ast_procesarCAT08(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id) {
               :int sizeFSPEC=0;
               :struct datablock_plot dbp;
               :
               :    dbp.tod_stamp = current_time; dbp.id = id; dbp.index = 0;
               :    sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
               :
     1  0.0124 :    if ( (ptr_raw[0] & 128) && // sac/sic
               :	(ptr_raw[0] & 64) &&   // msg type
               :	(ptr_raw[0] & 1) &&    // FX
               :	(ptr_raw[1] & 128) ) { // timeofday
               :
               :	dbp.cat = CAT_08;
               :	dbp.type = NO_DETECTION;
               :	dbp.sac = ptr_raw[sizeFSPEC]; dbp.sic = ptr_raw[sizeFSPEC+1];
               :	
               :	if (ptr_raw[sizeFSPEC + 2] & 254) {
               :	    dbp.type = TYPE_C8_CONTROL_SOP;
               :	}
               :	if (ptr_raw[sizeFSPEC + 2] & 255) {
               :	    dbp.type = TYPE_C8_CONTROL_EOP;
               :	}
               :	if ( (dbp.type == TYPE_C8_CONTROL_SOP) || 
               :	     (dbp.type == TYPE_C8_CONTROL_EOP) ) {
               :	    //start of picture or end of picture
               :	    dbp.available = IS_TOD | IS_TYPE | IS_SACSIC;
     1  0.0124 :	    dbp.tod = ((float)(ptr_raw[sizeFSPEC + 3]<<16) + 
               :			(ptr_raw[sizeFSPEC + 4]<<8) + 
               :			(ptr_raw[sizeFSPEC + 5])) / 128.0;
               :		
               :	    ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);
               :	    if (sendto(s, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
               :		log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
               :		exit(EXIT_FAILURE);
               :	    }
               :	}
               :    }
               :    return T_OK;
               :}
               :
               :
     1  0.0124 :void ttod_put_full(unsigned char sac, unsigned char sic, unsigned char *ptr_full_tod) {
               :int i=0;
               :
               :    while ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) &&
               :	    (full_tod[i] != 0) &&
               :	    (full_tod[i+1] != 0) &&
               :	    (full_tod[i] != sac) && 
     3  0.0371 :	    (full_tod[i+1] != sic) )
               :	    i+=TTOD_WIDTH;
               :
     2  0.0248 :    if ( i < MAX_RADAR_NUMBER*TTOD_WIDTH ) {
               :	if (full_tod[i] == 0) {
               :	    full_tod[i] = sac;
               :	    full_tod[i+1] = sic;
               :	}
     2  0.0248 :	full_tod[i+2] = ptr_full_tod[0];
               :	full_tod[i+3] = ptr_full_tod[1];
               :	full_tod[i+4] = ptr_full_tod[2];
               :	
     2  0.0248 :	full_tod[i+5] = ptr_full_tod[1];
               :	full_tod[i+6] = ptr_full_tod[2];
               :    }
               :    return;
               :}
               :
               :float ttod_get_full(int sac, int sic, unsigned char *ptr_ttod) {
     3  0.0371 :int i = 0;
     4  0.0495 :int adjust = 0;
               :
               :    while ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) &&
               :	    (full_tod[i] != 0) &&
               :	    (full_tod[i+1] != 0) &&
               :	    (full_tod[i] != (unsigned char) sac) && 
    51  0.6314 :	    (full_tod[i+1] != (unsigned char) sic) )
               :	    i+=TTOD_WIDTH;
               :
    52  0.6438 :    if ( (i < MAX_RADAR_NUMBER*TTOD_WIDTH) && (full_tod[i] != 0) && (full_tod[i+1] != 0) ) {
               :	long p,q;
               :	
    27  0.3343 :	p = (full_tod[i+5]<<8) + full_tod[i+6];
    11  0.1362 :	q = (ptr_ttod[0]<<8) + ptr_ttod[1];
               :
    31  0.3838 :	if ( abs(q-p) < 128) {
    12  0.1486 :	    full_tod[i+5] = ptr_ttod[0];
    10  0.1238 :	    full_tod[i+6] = ptr_ttod[1];
     1  0.0124 :	} else {
               :	    adjust=1;
               :	}
               :
   122  1.5105 :	return ((float)( ((full_tod[i+2] + adjust)<<16) + q))/128.0;
               :
               :    }
               :    return T_ERROR;
     3  0.0371 :};
               :
/* 
 * Total samples for file : "/root/prog/conversor_asterix2/src/asterix.c"
 * 
 *   7417 91.8286
 */


/* 
 * Command line: opannotate --source --output-dir=anot1 ./reader_network 
 * 
 * Interpretation of command line:
 * Output annotated source file with samples
 * Output all files
 * 
 * CPU: PIII, speed 747.746 MHz (estimated)
 * Counted CPU_CLK_UNHALTED events (clocks processor is not halted) with a unit mask of 0x00 (No unit mask) count 6000
 */
