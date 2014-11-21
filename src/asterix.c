/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2014 Diego Torres <diego dot torres at gmail dot com>

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

extern float current_time_today;
extern int s_output_multicast;
extern struct sockaddr_in srvaddr;

unsigned char full_tod[MAX_RADAR_NUMBER*TTOD_WIDTH]; /* 2 sacsic, 1 null, 3 full_tod, 2 max_ttod */

// return true if filter_type found (datablock should be left out)
bool filter_test(unsigned char *ptr_raw, int ptr, int filter_type) {

    if (filter_type == FILTER_NONE)
        return true;

    if ((filter_type & FILTER_GROUND) == FILTER_GROUND) {
        if ( ((ptr_raw[ptr] & 28) == 4) ||
             ((ptr_raw[ptr] & 28) == 12) ) {
                return true;
        }
    }
    return false;
}

void ast_output_datablock(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, unsigned long index) {
int i;
char *ptr_tmp;

    ptr_tmp = (char *) mem_alloc(size_datablock*3 + 2);
    memset(ptr_tmp, 0x0, size_datablock*3 + 2);

    for (i=0; i < size_datablock*3; i+=3)
	snprintf((char *)(ptr_tmp + i), size_datablock*3 + 1, "%02X ", (unsigned char) (ptr_raw[i/3]));

    ptr_tmp[size_datablock*3+1] = '\0';

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

char * decode_bds30(unsigned char * ptr_raw, int j, struct datablock_plot dbp) {

    char ptr_260[3*7+1]; int i;
    for( i = 0; i < 7; i++ ) sprintf((char *)(ptr_260 + i*3), "%02X ", (unsigned char) (ptr_raw[j+i]));
    ptr_260[3*7-1] = '\0';
/*
    log_printf(LOG_NORMAL, "I048/260 sac(%d) sic(%d) modes(%06X) modea(%04o%s%s%s) aid(%s) rho(%3.3f) theta(%3.3f) tod(%3.3f)\n", 
        dbp.sac, dbp.sic,
        (dbp.available & IS_MODES_ADDRESS) ? dbp.modes_address : 0,
        (dbp.available & IS_MODEA) ? dbp.modea : 0,
        (dbp.modea_status & STATUS_MODEA_GARBLED) ? "G" : "",
        (dbp.modea_status & STATUS_MODEA_NOTVALIDATED) ? "I" : "",
        (dbp.modea_status & STATUS_MODEA_SMOOTHED) ? "S" : "",
        (dbp.available & IS_AIRCRAFT_ID) ? dbp.aircraft_id : (unsigned char*) "",
        dbp.rho, dbp.theta, dbp.tod);
*/
    {
        int bds1 = 0; // 4   0..4        0         0
        int bds2 = 0; // 4   5..8        0         4
        int ara = 0;  //     9..22       1,2       0
        int ara41 = 0; //     9           1            bit41
        int ara42 = 0; //     10          1            bit42
        int ara43 = 0; //     11          1            bit43
        int ara44 = 0; //     12          1            bit44
        int ara45 = 0; //     13          1            bit45
        int ara46 = 0; //     14          1            bit46
        int ara47 = 0; //     15          1            bit47

        int rac  = 0;  // 4   23..26      2,3
        int rac1 = 0;  //     23          2            bit55
        int rac2 = 0;  //     24          2            bit55
        int rac3 = 0;  //     25          3            bit55
        int rac4 = 0;  //     26          3            bit55
        int rat = 0;  // 1   27          3
        int mte = 0;  // 1   28          3
        int tti = 0;  // 2   29..30      3
        //int tid = 0; // 26   31..56      3,4,5,6
        int tid_ms = 0;// 26   31..54    3,4,5,6
        int tid_modec = 0; //13 31..43      3,4,5      bit63..bit75
        int tid_range = 0; // 7     44..50        5,6       bit76..bit82
        float tid_rangef = 0.0;

        int tid_bearing = 0.0;

        bds1 = (ptr_raw[j + 0] & 0xF0) >> 4;
        bds2 = (ptr_raw[j + 0] & 0x0F);
        ara41 = (ptr_raw[j + 1] & 0x80) >> 7;
        ara42 = (ptr_raw[j + 1] & 0x40) >> 6;
        ara43 = (ptr_raw[j + 1] & 0x20) >> 5;
        ara44 = (ptr_raw[j + 1] & 0x10) >> 4;
        ara45 = (ptr_raw[j + 1] & 0x08) >> 3;
        ara46 = (ptr_raw[j + 1] & 0x04) >> 2;
        ara47 = (ptr_raw[j + 1] & 0x02) >> 1;
        // ara41 = 0 There is more than one threat and the RA is intended to provide separation
        // below some threat(s) and above some other threat(s) or no RA has been
        // generated (when MTE = 0)
        // ara41 = 1 Either there is only one threat or the RA is intended to provide separation in
        // the same direction for all threats.
        mte = (ptr_raw[j + 3] & 0x10) >> 4;
        // mte = 0 One threat is being processed by the resolution logic (when ARA bit 41 = 1); or no threat is
        // being processed by the resolution logic (when ARA bit 41 = 0)
        // color orange
        // mte = 1 Two or more simultaneous threats are being processed by the resolution logic
        // color red
        if (ara41==1 && mte==0) { // one threat is being processed by the resolution logic
            /*  Bit Coding
                42 0 RA is preventive  // color orange
                   1 RA is corrective  // color red
                43 0 Upward sense RA has been generated // up arrow
                   1 Downward sense RA has been generated // down arrow
                44 0 RA is not increased rate
                   1 RA is increased rate
                45 0 RA is not a sense reversal
                   1 RA is a sense reversal
                46 0 RA is not altitude crossing
                   1 RA is altitude crossing
                47 0 RA is vertical speed limit
                   1 RA is positive
                48-54 Reserved for ACAS III
            */
            ara = ara42<<5 | ara43<<4 | ara44<<3 | ara45<<2 | ara46<<1 | ara47;
        } else if (ara41==0 && mte==1) { // RA is intended to provide separation below some threat(s) and above some other threat(s)
            /*  Bit Coding
                42 0 RA does not require a correction in the upward sense
                   1 RA requires a correction in the upward sense
                43 0 RA does not require a positive climb
                   1 RA requires a positive climb
                44 0 RA does not require a correction in the downward sense
                   1 RA requires a correction in the downward sense
                45 0 RA does not require a positive descend
                   1 RA requires a positive descend
                46 0 RA does not require a crossing
                   1 RA requires a crossing
                47 0 RA is not a sense reversal
                   1 RA is a sense reversal
                48-54 Reserved for ACAS III
            */
            ara = ara42<<5 | ara43<<4 | ara44<<3 | ara45<<2 | ara46<<1 | ara47;
            } else if (ara41==0 && mte==0) {
                // no vertical RA has been generated
                // color green
                ara = 0;
            }
            //log_printf(LOG_NORMAL, "%08X %08X\n", bds30->bds1, (ptr_raw+j));
            /* received from other ACAS aircraft. The bits in RAC shall have the following meanings:
                Bit Resolution advisory complement
                55 Do not pass below
                56 Do not pass above
                57 Do not turn left
                58 Do not turn right
                A bit set to 1 shall indicate that the associated RAC is active. A bit set to 0 shall indicate that the associated RAC is inactive.
            */
            rac1 = (ptr_raw[j + 2] & 0x02) >> 1;
            rac2 = (ptr_raw[j + 2] & 0x01);
            rac3 = (ptr_raw[j + 3] & 0x80) >> 7;
            rac4 = (ptr_raw[j + 3] & 0x40) >> 6;
            rac = rac1<<3 | rac2<<2 | rac3<<1 | rac4;

            /*
                This 1-bit (59) subfield shall indicate when an RA previously generated
                by ACAS has ceased being generated. After an RA has been terminated by ACAS, it is still required to be reported by the Mode S transponder for 18±1 s
                (4.3.11.4.1). The RA terminated indicator may be used, for example, to permit timely removal of an RA indication from an air
                traffic controller’s display, or for assessments of RA duration within a particular airspace.
            */
            rat = (ptr_raw[j + 3] & 0x20) >> 5;
            /*
                TTI (threat type indicator subfield). This 2-bit subfield (61-62) shall define the type of identity data
                contained in the TID subfield.
                Coding
                    0 No identity data in TID
                    1 TID contains a Mode S transponder address
                    2 TID contains altitude, range and bearing data
                    3 Not assigned
            */
            tti = (ptr_raw[j + 3] & 0x0C) >> 2;
            switch (tti) {
                case 0:
                    break;
                case 1:
                    tid_ms = (ptr_raw[j+3] & 0x03) << 6;
                    tid_ms |= (ptr_raw[j+4] & 0xFC) >> 2;
                    tid_ms = tid_ms << 8;
                    tid_ms |= (ptr_raw[j+4] & 0x03) << 6;
                    tid_ms |= (ptr_raw[j+5] & 0xFC) >> 2;
                    tid_ms = tid_ms << 8;
                    tid_ms |= (ptr_raw[j+5] & 0x03) << 6;
                    tid_ms |= (ptr_raw[j+6] & 0xFC) >> 2;
                    //log_printf(LOG_NORMAL, "%06X\n", tid_ms);
                    //tid_ms = tid_ms << 8;
                    if ((ptr_raw[j+6] & 0x03) != 0) {
                        log_printf(LOG_ERROR, "error with Threat Identity Data subfield in bds3,0, bit87 & 88 should be zero\n");
                    }
                    break;
                case 2: {
                    int a1 = 0, a2 = 0, a4 = 0;
                    int b1 = 0, b2 = 0, b4 = 0;
                    int c1 = 0, c2 = 0, c4 = 0;
                    int d1 = 0, d2 = 0, d4 = 0;
                    int tidr76 = 0, tidr77 = 0, tidr78 = 0;
                    int tidr79 = 0, tidr80 = 0, tidr81 = 0;
                    int tidr82 = 0;
                    int tidb83 = 0, tidb84 = 0, tidb85 = 0;
                    int tidb86 = 0, tidb87 = 0, tidb88 = 0;
                
                    c1 = (ptr_raw[j+3] & 0x02) >> 1;
                    a1 = (ptr_raw[j+3] & 0x01);
                    c2 = (ptr_raw[j+4] & 0x80) >> 7;
                    a2 = (ptr_raw[j+4] & 0x40) >> 6;
                    c4 = (ptr_raw[j+4] & 0x20) >> 5;
                    a4 = (ptr_raw[j+4] & 0x10) >> 4;
                    b1 = (ptr_raw[j+4] & 0x04) >> 2;
                    d1 = (ptr_raw[j+4] & 0x02) >> 1;
                    b2 = (ptr_raw[j+4] & 0x01);
                    d2 = (ptr_raw[j+5] & 0x80) >> 7;
                    b4 = (ptr_raw[j+5] & 0x40) >> 6;
                    d4 = (ptr_raw[j+5] & 0x20) >> 5;
                    tid_modec = d1<<11 | d2<<10 | d4<<9 | a1<<8 | a2<<7 | a4<<6 | b1<<5 | b2<<4 | b4<<3 | c1<<2 | c2<<1 | c4;
                    //Format MSB to LSB: D2 D4 A1 A2 A4 B1 B2 B4 C1 C2 C4
                    tid_modec ^= (tid_modec >> 8);
                    tid_modec ^= (tid_modec >> 4);
                    tid_modec ^= (tid_modec >> 2);
                    tid_modec ^= (tid_modec >> 1);
                    tid_modec -= (((tid_modec >> 4) * 6) + ((((tid_modec) % 16) / 5) * 2));
                    tid_modec = (tid_modec - 13)*100;

                    tidr76 = (ptr_raw[j+5] & 0x10) >> 4;
                    tidr77 = (ptr_raw[j+5] & 0x08) >> 3;
                    tidr78 = (ptr_raw[j+5] & 0x04) >> 2;
                    tidr79 = (ptr_raw[j+5] & 0x02) >> 1;
                    tidr80 = (ptr_raw[j+5] & 0x01);
                    tidr81 = (ptr_raw[j+6] & 0x80) >> 7;
                    tidr82 = (ptr_raw[j+6] & 0x40) >> 6;

                    tid_range = tidr76 << 6 | tidr77 << 5 | tidr78 << 4 | 
                        tidr79 << 3 | tidr80 << 2 | tidr81 << 1 | tidr82;
                    if (tid_range == 0)
                        tid_rangef = 0;
                    else if (tid_range == 1)
                        tid_rangef = 0.05;
                    else if (tid_range == 127)
                        tid_rangef = 12.55;
                    else
                        tid_rangef = (tid_range-1.0)/10.0;

                    tidb83 = (ptr_raw[j+6] & 0x20) >> 5;
                    tidb84 = (ptr_raw[j+6] & 0x10) >> 4;
                    tidb85 = (ptr_raw[j+6] & 0x08) >> 3;
                    tidb86 = (ptr_raw[j+6] & 0x04) >> 2;
                    tidb87 = (ptr_raw[j+6] & 0x02) >> 1;
                    tidb88 = (ptr_raw[j+6] & 0x01);

                    tid_bearing = tidb83 << 5 | tidb84 << 4 | tidb85 << 3 |
                        tidb86 << 2 | tidb87 << 1 | tidb88;
                            
                    if (tid_bearing >= 1 && tid_bearing <=60)
                        tid_bearing = (tid_bearing-1) * 6;
                    break;
                }
                case 3:
                    default:
                    break;
            }
            /*
            if (tti == 1) {
                log_printf(LOG_NORMAL, "\t048/260 [1] bds1(%d) bds2(%d) ara41(%d) ara(%02X) rac(%02X) mte(%d) rat(%d) tti(%d) ms[%06X] [%s]\n", bds1, bds2, ara41, ara, rac, mte, rat, tti, tid_ms, ptr_260);
            } else if (tti == 2) {
                log_printf(LOG_NORMAL, "\t048/260 [2] bds1(%d) bds2(%d) ara41(%d) ara(%02X) rac(%02X) mte(%d) rat(%d) tti(%d) modec(%04d) range(%3.2fNM) bearing(%03dº) [%s]\n", bds1, bds2, ara41, ara, rac, mte, rat, tti, tid_modec, tid_rangef, tid_bearing, ptr_260);
            } else {
                log_printf(LOG_NORMAL, "\t048/260 [0] bds1(%d) bds2(%d) ara41(%d) ara(%02X) rac(%02X) mte(%d) rat(%d) tti(%d) [%s]\n", bds1, bds2, ara41, ara, rac, mte, rat, tti, ptr_260);
            }
            */
        }

    return NULL;

}


int ast_procesarCAT01(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
int size_current = 0, j = 0;
int index = 0;

    do {
	int sizeFSPEC;
	struct datablock_plot dbp;

	memset(&dbp, 0, sizeof(struct datablock_plot));
//	log_printf(LOG_NORMAL, "fspec %02X\n", ptr_raw[0]);

	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
	dbp.cat = CAT_01;
	dbp.available = IS_ERROR;
	dbp.type = NO_DETECTION;
	dbp.plot_type = IS_ERROR;
	dbp.modea_status = 0;
	dbp.modec_status = 0;
	dbp.flag_test = 0;
	dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = index;
        dbp.rho = 0;
        dbp.theta = 0;
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
		//log_printf(LOG_NORMAL, "type %02X TRACK\n", ptr_raw[j]);
	        dbp.available |= IS_TRACK;
		size_current = size_datablock - 3; //exit without further decompression

	    } else { // plot
		dbp.available |= IS_TYPE;
		//log_printf(LOG_NORMAL, "type %02X\n", ptr_raw[j]);

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
		    dbp.type |= TYPE_C1_FIXED_TRANSPONDER;
		}
		if (ptr_raw[j] & 1) {
		    j++; size_current++;
		    if (ptr_raw[j] & 128) { dbp.flag_test = 1; }
		    while (ptr_raw[j] & 1) { j++; size_current++;}
		}
		size_current++; j++;
		if ( ptr_raw[0] & 32 ) { //I001/040
		    //log_printf(LOG_NORMAL, "polar %02X %02X %02X %02X \n", ptr_raw[j], ptr_raw[j+1], ptr_raw[j+2], ptr_raw[j+3] );
		    dbp.rho = (ptr_raw[j]*256 + ptr_raw[j+1]) / 128.0;
		    dbp.theta = (ptr_raw[j+2]*256 + ptr_raw[j+3]) * 360.0/65536.0;
		    size_current += 4; j+= 4;
		    dbp.available |= IS_MEASURED_POLAR;
	        }
		if ( ptr_raw[0] & 16 ) { //I001/070
		    //log_printf(LOG_NORMAL, "modea %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
		    dbp.modea_status |= (ptr_raw[j] & 128) ? STATUS_MODEA_NOTVALIDATED : 0;
		    dbp.modea_status |= (ptr_raw[j] & 64) ? STATUS_MODEA_GARBLED : 0;
		    dbp.modea_status |= (ptr_raw[j] & 32) ? STATUS_MODEA_SMOOTHED : 0;
		    dbp.modea = (ptr_raw[j] & 15)*256 + ptr_raw[j+1];
		    size_current += 2; j += 2;
		    dbp.available |= IS_MODEA;
		}
		if ( ptr_raw[0] & 8  ) { //I001/090
		    //log_printf(LOG_NORMAL, "modec %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
		    dbp.modec_status |= (ptr_raw[j] & 128) ? STATUS_MODEC_NOTVALIDATED : 0;
		    dbp.modec_status |= (ptr_raw[j] & 64) ? STATUS_MODEC_GARBLED : 0;
		    dbp.modec = ( (ptr_raw[j] & 63)*256 + ptr_raw[j+1]) * 1.0/4.0;// * 100;
		    size_current += 2; j += 2;
		    dbp.available |= IS_MODEC;
		}
		if ( ptr_raw[0] & 4  ) { //I001/130
		    //log_printf(LOG_NORMAL, "responses %02X\n", ptr_raw[j]);
		    dbp.radar_responses = (ptr_raw[j] >> 1);
		    while (ptr_raw[j] & 1) { j++; size_current++; }
		    size_current++; j++;
		    dbp.available |= IS_RADAR_RESPONSES;
		}
		if ( ptr_raw[0] & 2  ) { //I001/141
		    //log_printf(LOG_NORMAL, "tod %02X %02X\n", ptr_raw[j], ptr_raw[j+1]);
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
		if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT001
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


    memset(&dbp, 0, sizeof(struct datablock_plot));
    dbp.plot_type = IS_ERROR;
    dbp.tod_stamp = current_time_today;
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
            if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT002
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

    memset(&dbp, 0, sizeof(struct datablock_plot));
    dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = 0;
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
		if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT008
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
	memset(&dbp, 0, sizeof(struct datablock_plot));
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
	dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;
	dbp.rho = 0;
	dbp.theta = 0;

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
	if ( ptr_raw[0] & 4 ) { /* I010/040 */
            dbp.rho = (ptr_raw[j]*256 + ptr_raw[j+1]);
	    dbp.theta = (ptr_raw[j+2]*256 + ptr_raw[j+3]) * 360.0/65536.0;
	    size_current += 4; j+= 4;
	    dbp.available |= IS_MEASURED_POLAR;
	}
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
		if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT010
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

    memset(&dbp, 0, sizeof(struct datablock_plot));
    dbp.plot_type = IS_ERROR;
    dbp.tod_stamp = current_time_today;
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
            if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT019
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
	memset(&dbp, 0, sizeof(struct datablock_plot));
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
	dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;

	//ast_output_datablock(ptr_raw, size_datablock - 3, dbp.id, dbp.index);
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
	    switch(ptr_raw[j] >> 1) {
	        case 64: dbp.type = TYPE_C20_NONMODESMLAT; break;
	        case 32: dbp.type = TYPE_C20_MODESMLAT; break;
	        case 16: dbp.type = TYPE_C20_HFMLAT; break;
                case 8: dbp.type = TYPE_C20_VDLMLAT; break;
                case 4: dbp.type = TYPE_C20_UATMLAT; break;
                case 2: dbp.type = TYPE_C20_DMEMLAT; break;
                case 1: dbp.type = TYPE_C20_OTHERMLAT; break;
                default: dbp.type = IS_ERROR; break;
            }
            if (ptr_raw[j] & 1) { 
                j++; size_current++;
                if ((ptr_raw[j] & 128) == 128) dbp.type |= TYPE_C20_RAB;
                if ((ptr_raw[j] & 64) == 128) dbp.type |= TYPE_C20_SPI;
                if ((ptr_raw[j] & 32) == 128) dbp.type |= TYPE_C20_CHN;
                if ((ptr_raw[j] & 16) == 128) dbp.type |= TYPE_C20_GBS;
                if ((ptr_raw[j] & 8) == 128) dbp.type |= TYPE_C20_CRT;
                if ((ptr_raw[j] & 4) == 128) dbp.type |= TYPE_C20_SIM;
                if ((ptr_raw[j] & 2) == 128) dbp.type |= TYPE_C20_TST;
            }
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
	if ( ptr_raw[0] & 8 ) { /* I020/042 */ 
            dbp.x = (ptr_raw[j+0]*256*256 + ptr_raw[j+1]*256 + ptr_raw[j+2]) / 0.5;
	    dbp.y = (ptr_raw[j+3]*256*256 + ptr_raw[j+4]*256 + ptr_raw[j+5]) / 0.5;
	    j += 6; size_current += 6; 
	    dbp.available |= IS_MEASURED_CARTE;
	}
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
		if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT020
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
	memset(&dbp, 0, sizeof(struct datablock_plot));
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
	dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = index;
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
		if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT021
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
	memset(&dbp, 0, sizeof(struct datablock_plot));
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
	dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = index;
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
		if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT034
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

int ast_procesarCAT48F(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar, filter_struct *fs) {
int size_current = 0, j = 0;
int index = 0;
unsigned char *datablock_start = NULL;

    if (fs !=NULL && fs->filter_type != FILTER_NONE) {
        fs->ptr_raw = (unsigned char *) mem_alloc(size_datablock);
        fs->size_datablock = 3;
    }

    do {
	int sizeFSPEC;
	struct datablock_plot dbp;
        bool filter_true = false;
	memset(&dbp, 0, sizeof(struct datablock_plot));
	sizeFSPEC = ast_get_size_FSPEC(ptr_raw, size_datablock);
	dbp.cat = CAT_48;
	dbp.available = IS_ERROR;
	dbp.bds_available = BDS_EMPTY;
	dbp.type = NO_DETECTION;
	dbp.plot_type = IS_ERROR;
	dbp.modea_status = 0;
	dbp.modec_status = 0;
	dbp.flag_test = 0;
	dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = index;
	dbp.radar_responses = 0;
	dbp.rho = 0;
	dbp.theta = 0;
	dbp.modes_address = 0;
	memset(dbp.aircraft_id, 0, 9);
	memset(dbp.bds_10, 0, 7);
	memset(dbp.bds_17, 0, 7);
	memset(dbp.bds_30, 0, 7);
	dbp.di048_230_com = 0; //      CAT48
        dbp.di048_230_mssc = 0;//      CAT48
	dbp.di048_230_arc = 0; //      CAT48
	dbp.di048_230_aic = 0; //      CAT48
	dbp.di048_230_b1a = 0; //      CAT48
	dbp.di048_230_b1b = 0; //      CAT48

//	if (sizeFSPEC == 0) {
//	    log_printf(LOG_WARNING, "ERROR_FSPEC_SIZE[%d] %s\n", sizeFSPEC, ptr_raw);
//	    return T_ERROR;
//	}

	//ast_output_datablock(ptr_raw, size_datablock - size_current - 3, id, index);

        datablock_start = ptr_raw;
	j = sizeFSPEC;
	size_current += sizeFSPEC;
	if ( ptr_raw[0] & 128 ) { //I048/010
	    dbp.sac = ptr_raw[sizeFSPEC]; dbp.sic = ptr_raw[sizeFSPEC + 1];
	    j += 2; size_current += 2; dbp.available |= IS_SACSIC;
	}
	if ( ptr_raw[0] & 64  ) { //I048/140
	    dbp.tod = ((float)(ptr_raw[j]*256*256 + ptr_raw[j+1]*256 + ptr_raw[j+2]))/128.0;
//	    log_printf(LOG_NORMAL, "hextod(%02X %02X %02X)\n", ptr_raw[j], ptr_raw[j+1], ptr_raw[j+2]);
	    size_current += 3; j += 3; dbp.available |= IS_TOD;
	}
	if ( ptr_raw[0] & 32 ) { /* I048/020 */
	    int b = 0;
	    b = (ptr_raw[j] & 224) >> 5;
	    switch (b) {
		case 0: dbp.type = NO_DETECTION; break;
		case 1: dbp.type = TYPE_C48_PSR; break;
		case 2: dbp.type = TYPE_C48_SSR; break;
		case 3: dbp.type = TYPE_C48_CMB; break;
		case 4: dbp.type = TYPE_C48_SSRSGEN; break;
		case 5: dbp.type = TYPE_C48_SSRSROL; break;
		case 6: dbp.type = TYPE_C48_CMBSGEN; break;
		case 7: dbp.type = TYPE_C48_CMBSROL; break;
		default: dbp.type = NO_DETECTION; break;
	    }
	    if (ptr_raw[j] & 2) dbp.type |= TYPE_C48_FIXED_TRANSPONDER;
	    while (ptr_raw[j] & 1) { j++; size_current++; } j++; size_current++; 
	}
	if ( ptr_raw[0] & 16 ) { /* I048/040 */
	    dbp.rho = (ptr_raw[j]*256 + ptr_raw[j+1]) / 256.0;
	    dbp.theta = (ptr_raw[j+2]*256 + ptr_raw[j+3]) * 360.0/65536.0;
	    size_current += 4; j+= 4;
	    dbp.available |= IS_MEASURED_POLAR;
	}
	if ( ptr_raw[0] & 8 ) {  /* I048/070 */
            dbp.modea_status |= (ptr_raw[j] & 128) ? STATUS_MODEA_NOTVALIDATED : 0;
	    dbp.modea_status |= (ptr_raw[j] & 64) ? STATUS_MODEA_GARBLED : 0;
	    dbp.modea_status |= (ptr_raw[j] & 32) ? STATUS_MODEA_SMOOTHED : 0;
	    dbp.modea = (ptr_raw[j] & 15)*256 + ptr_raw[j+1];
	    size_current += 2; j += 2;
	    dbp.available |= IS_MODEA;
	}
	if ( ptr_raw[0] & 4 ) {  /* I048/090 */ j += 2; size_current += 2; }
	if ( ptr_raw[0] & 2 ) {  /* I048/130 */ int k = j; //log_printf(LOG_NORMAL, "I048/130[%02X]\n", ptr_raw[j]);
	    if ( ptr_raw[k] & 128 ) { j++; size_current++; }
	    if ( ptr_raw[k] & 64 ) { j++; size_current++; }
	    if ( ptr_raw[k] & 32 ) { j++; size_current++; }
	    if ( ptr_raw[k] & 16 ) { j++; size_current++; }
	    if ( ptr_raw[k] & 8 ) { j++; size_current++; }
	    if ( ptr_raw[k] & 4 ) { j++; size_current++; }
	    if ( ptr_raw[k] & 2 ) { j++; size_current++; }
	    j++; size_current++;
	}
	if ( ptr_raw[0] & 1 ) { // FX1
	    if ( ptr_raw[1] & 128 ) { /* I048/220 */
	        dbp.modes_address = ptr_raw[j]<<16 | ptr_raw[j+1] <<8 | ptr_raw[j+2];
	        j += 3; size_current += 3;
	        dbp.available |= IS_MODES_ADDRESS;
	    }
	    if ( ptr_raw[1] & 64 ) {  /* I048/240 */
        	//char * ptr_tmp;
        	int i;
        	//ptr_tmp = (char *) mem_alloc(6*3 + 1);
	        //Bmemset(ptr_tmp, 0x0, 6*3 + 1);
	        //for( i = 0; i < 6; i++ ) sprintf((char *)(ptr_tmp + i*3), "%02X ", (unsigned char) (ptr_raw[j+i]));
                //log_printf(LOG_NORMAL, "I048/240[%s]\n", ptr_tmp);
                //mem_free(ptr_tmp);

	        dbp.aircraft_id[0] = ptr_raw[j] >> 2;
	        dbp.aircraft_id[1] = ((ptr_raw[j] & 0x03) << 4) | ((ptr_raw[j+1] & 0xF0) >> 4);
	        dbp.aircraft_id[2] = ((ptr_raw[j+1] & 0x0F) << 2) | ((ptr_raw[j+2] & 0xC0) >> 6);
	        dbp.aircraft_id[3] = (ptr_raw[j+2] & 0x3F);
	        dbp.aircraft_id[4] = ptr_raw[j+3] >> 2;
	        dbp.aircraft_id[5] = ((ptr_raw[j+3] & 0x03) << 4) | ((ptr_raw[j+4] & 0xF0) >> 4);
	        dbp.aircraft_id[6] = ((ptr_raw[j+4] & 0x0F) << 2) | ((ptr_raw[j+5] & 0xC0) >> 6);
	        dbp.aircraft_id[7] = (ptr_raw[j+5] & 0x3F);
	        dbp.aircraft_id[8] = 0;

	        for(i=0;i<8;i++) {
	            //log_printf(LOG_NORMAL, ">%02X<\n", dbp.aircraft_id[i]);
	            if (dbp.aircraft_id[i]>0 && dbp.aircraft_id[i]<=26) {
	                dbp.aircraft_id[i] += 64;
	            } else if (dbp.aircraft_id[i] == 32) {
	                dbp.aircraft_id[i] = 32;
	            } else if (dbp.aircraft_id[i]>47 && dbp.aircraft_id[i]<=57) {
	                continue; //dbp.aircraft_id[i] = dbp.aircraft_id[i];
	            } else {
	                dbp.aircraft_id[i] = 32;
	            }
	        }
	        for(i=7;i>=0;i--)
	            if (dbp.aircraft_id[i]==32)
	                dbp.aircraft_id[i]=0;
	            else
	                break;
                //log_printf(LOG_NORMAL, ">%s<\n", dbp.aircraft_id);

	        j += 6; size_current += 6;
                dbp.available |= IS_AIRCRAFT_ID;
	    }
	    if ( ptr_raw[1] & 32 ) {  /* I048/250 */ int k = j; j += ptr_raw[k]*8 + 1 ; size_current += ptr_raw[k]*8 + 1; }
	    if ( ptr_raw[1] & 16 ) {  /* I048/161 */ j += 2; size_current += 2; }
	    if ( ptr_raw[1] & 8 ) {   /* I048/042 */ j += 4; size_current += 4; }
	    if ( ptr_raw[1] & 4 ) {   /* I048/200 */ j += 4; size_current += 4; }
	    if ( ptr_raw[1] & 2 ) {   /* I048/170 */ while (ptr_raw[j] & 1) { j++; size_current++; } j++; size_current++; }
	    if ( ptr_raw[1] & 1 ) { // FX2
		if ( ptr_raw[2] & 128 ) { /* I048/210 */ j += 4; size_current += 4; }
		if ( ptr_raw[2] & 64 ) {  /* I048/030 */ while (ptr_raw[j] & 1) { j++; size_current++; } j++; size_current++; }
		if ( ptr_raw[2] & 32 ) {  /* I048/080 */ j += 2; size_current += 2; }
		if ( ptr_raw[2] & 16 ) {  /* I048/100 */ j += 4; size_current += 4; }
		if ( ptr_raw[2] & 8 ) {   /* I048/110 */ j += 2; size_current += 2; }
		if ( ptr_raw[2] & 4 ) {   /* I048/120 */
		    int k = j;
		    if ( ptr_raw[k] & 128 ) { j += 2; size_current += 2; }
		    if ( ptr_raw[k] & 64 ) { int l = j; j += ptr_raw[l]*6 + 1; size_current += ptr_raw[l]*6 + 1; }
		}
		if ( ptr_raw[2] & 2 ) { /* I048/230 */
                    //log_printf(LOG_ERROR, "en I048/230\n");
                    if (fs != NULL) filter_true = filter_test(ptr_raw, j, fs->filter_type);
                    dbp.di048_230_com = (ptr_raw[j] & 0xe0) >> 5;
                    dbp.di048_230_mssc = (ptr_raw[j+1] & 0x80) >> 7;
                    dbp.di048_230_arc = (ptr_raw[j+1] & 0x40) >> 6;
                    dbp.di048_230_aic = (ptr_raw[j+1] & 0x20) >> 5;
                    dbp.di048_230_b1a = (ptr_raw[j+1] & 0x10) >> 4;
                    dbp.di048_230_b1b = (ptr_raw[j+1] & 0x0f);
                    dbp.available |= IS_COMM_CAP;
                    j += 2; size_current += 2;
                }
		if ( ptr_raw[2] & 1 ) { // FX3
		    if ( ptr_raw[3] & 128 ) { /* I048/260 */ 
/*		        struct bds30s {
		            unsigned bds1 :4;
		            unsigned bds2 :4;
		            unsigned ara  :14;
		            unsigned rac  :4;
		            unsigned ra_terminated :1;
		            unsigned multiple :1;
		            unsigned thread_type :2;
		            unsigned thread_identity :26;
		        };
                        //struct bds30s *bds30 = (struct bds30s *) (ptr_raw + j);
*/
                        memcpy(dbp.bds_30, ptr_raw + j, 7);
                        dbp.bds_available |= BDS_30;
                        //decode_bds30(dbp.bds_30, 0, dbp);
                        
                        //bds(%08X)\n", bds30->bds1);
                        //log_printf(LOG_NORMAL, "I048/260 bds(%014X)\n", bds30->ara);
                        
		        j += 7; size_current += 7; 
		        //log_printf(LOG_ERROR, "DETECTED I048/260 in %ld !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", id);
		    }
		    if ( ptr_raw[3] & 64 ) {  /* I048/055 */ j += 1; size_current += 1; }
		    if ( ptr_raw[3] & 32 ) {  /* I048/050 */ j += 2; size_current += 2; }
		    if ( ptr_raw[3] & 16 ) {  /* I048/065 */ j += 1; size_current += 1; }
		    if ( ptr_raw[3] & 8 ) {   /* I048/060 */ j += 2; size_current += 2; }
		    if ( ptr_raw[3] & 4 ) {   /* I048/SP  */ size_current = size_datablock - 3; j = 0;
			log_printf(LOG_ERROR, "unexpected I048/SP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");exit(EXIT_FAILURE);
		    }
		    if ( ptr_raw[3] & 2 ) {   /* I048/RE  */ size_current = size_datablock - 3; j = 0;
			log_printf(LOG_ERROR, "unexpected I048/RE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");exit(EXIT_FAILURE);
		    }
		    if ( ptr_raw[3] & 1 ) {   /* FX4 */      size_current = size_datablock - 3; j = 0;
			log_printf(LOG_ERROR, "unexpected I048/FX4!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");exit(EXIT_FAILURE);
		    }
		}
	    }
	}

	if ( dbp.available & IS_TOD ) {
//	    log_printf(LOG_NORMAL, "currenttod(%3.3f) plottod(%3.3f) diff(%3.3f)\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
//	    if ((dbp.tod_stamp - dbp.tod < 0) || ((dbp.tod_stamp - dbp.tod > 5)) ) {
//		log_printf(LOG_NORMAL, "%3.3f %3.3f %3.3f\n", dbp.tod_stamp, dbp.tod, dbp.tod_stamp - dbp.tod);
//		exit(EXIT_FAILURE);
//	    }
            /*
            if (dbp.bds_available & BDS_30) {
                log_printf(LOG_NORMAL, "sac(%d) sic(%d) modes(%06X) a_id(%s) modea(%04o%s%s%s) rho(%3.3f) theta(%3.3f) tod(%3.3f)\n\t048/230(%s[%01X %01X %01X %01X %01X %01X])\n\n", 
                    dbp.sac, dbp.sic,
                    (dbp.available & IS_MODES_ADDRESS) ? dbp.modes_address : 0,
                    (dbp.available & IS_AIRCRAFT_ID) ? dbp.aircraft_id : (unsigned char*) "",
                    (dbp.available & IS_MODEA) ? dbp.modea : 0,
                    (dbp.modea_status & STATUS_MODEA_GARBLED) ? "G" : "",
                    (dbp.modea_status & STATUS_MODEA_NOTVALIDATED) ? "I" : "",
                    (dbp.modea_status & STATUS_MODEA_SMOOTHED) ? "S" : "",
                    dbp.rho, dbp.theta, dbp.tod,
                    (dbp.available & IS_COMM_CAP) ? "OK" : "NO",
                    (dbp.available & IS_COMM_CAP) ? dbp.di048_230_com : 0,
                    (dbp.available & IS_COMM_CAP) ? dbp.di048_230_mssc : 0,
                    (dbp.available & IS_COMM_CAP) ? dbp.di048_230_arc : 0,
                    (dbp.available & IS_COMM_CAP) ? dbp.di048_230_aic : 0,            
                    (dbp.available & IS_COMM_CAP) ? dbp.di048_230_b1a : 0,
                    (dbp.available & IS_COMM_CAP) ? dbp.di048_230_b1b : 0);
            }
            */
	    if (enviar) {
		if (sendto(s_output_multicast, &dbp, sizeof(dbp), 0, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) { // CAT048
		    log_printf(LOG_ERROR, "ERROR sendto: %s\n", strerror(errno));
		}
	    } else {
		update_calculations(dbp);
	    }
	}

        if ( fs != NULL && fs->filter_type != FILTER_NONE && !filter_true ) { // si filter_true == true, contiene un mensaje de ground, asi que no lo queremos
            memcpy(fs->ptr_raw + fs->size_datablock, datablock_start, j);
            fs->size_datablock += j;
        }
/*
        if (filter_true && (dbp.type == TYPE_C48_PSR)) {
            ast_output_datablock(datablock_start, size_datablock - size_current - 3, id, index);
            log_printf(LOG_ERROR, "ERROR: esto no puede pasar!!!\n"); exit(EXIT_FAILURE);
        }
*/
/*
        if ( fs != NULL && fs->filter_type != FILTER_NONE && filter_true ) {
            ast_output_datablock(datablock_start, size_current, id, 0); // + size_datablock - 3, id, index);
        }
*/
        ptr_raw += j;
        index++;

    } while ((size_current + 3) < size_datablock);

    if ( fs != NULL && (fs->filter_type != FILTER_NONE) && (fs->size_datablock != 3) ) {
        fs->ptr_raw[0] = (unsigned short) 48;
        fs->ptr_raw[1] = (unsigned short) (fs->size_datablock>>8);
        fs->ptr_raw[2] = (unsigned short) (fs->size_datablock & 255);
        //ast_output_datablock(fs->ptr_raw, fs->size_datablock, id, 0);
    } else if (fs != NULL && fs->size_datablock == 3) {
        fs->size_datablock = 0;
        mem_free(fs->ptr_raw);
    }

    return T_OK;
}

int ast_procesarCAT62(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar) {
/*int sizeFSPEC=0;
struct datablock_plot dbp;

    dbp.tod_stamp = current_time_today; dbp.id = id; dbp.index = 0;
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
	} else if ( (d > 0xB800) && (d <= 0xC000) && (full_tod[i+2] == 0x00) ) {// fuera de ventana de seguimiento
	    // rollover del tod. en este caso, ha habido paso por norte, pero el plot viene fechado con el paso
	    // por norte anterior. hace falta "ajustar temporalmente" para usar 0xA8 en lugar de 0x00.
	    //NRT  [000005] [5->01:00:00.039]
	    //PLT    [BFE8] p:5 q:49128 q-p:49123
	    //ADJ3 [00BFE8] [49128->01:06:23.812] ERROR
	    adjust=0xA8;
	    //log_printf(LOG_VERBOSE, "\tADJ3 %02X%02X%02X %ld/128.0\n", full_tod[i+2] + adjust, ptr_ttod[0], ptr_ttod[1], ((full_tod[i+2]+adjust)<<16) + q);
        } else if ( d > 0xF800 ) { 	// se ha dado la vuelta al contador, respetamos 0xFFFF - 16*128
	    if ( (q-p) > 0 ) { 		// es positivo si es un plot que llega tarde
		adjust = -1;
		    
		//Fri Jul  6 07:15:12 2007
		//    SAC: 20 SIC: 133
		//    NRT 330007 3342343
		//    PLT    FFED p:29 q:65517 q-p:65488
		//    ADJ3 32FFED 3342317/128.0
	    	//log_printf(LOG_VERBOSE, "\tADJ4 %02X%02X%02X %ld/128.0\n", full_tod[i+2] + adjust, ptr_ttod[0], ptr_ttod[1], ((full_tod[i+2]+adjust)<<16) + q);
	    } else {			// es negativo si todavia no ha llegado el paso por norte
		adjust = 1;
		//log_printf(LOG_VERBOSE, "\tADJ5 %02X%02X%02X %ld/128.0\n", full_tod[i+2] + adjust, ptr_ttod[0], ptr_ttod[1], ((full_tod[i+2]+adjust)<<16) + q);
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

	    log_printf(LOG_ERROR, "ADJ6 [%02X%02X%02X] [%ld->%s.%03.0f] ERROR\n", full_tod[i+2], ptr_ttod[0], ptr_ttod[1], 
		((full_tod[i+2]<<16) + q) , str, (((full_tod[i+2]<<16) + q)/128.0 - secs)*1000);
	    
	    log_printf(LOG_ERROR, "-------------------------------------------------------------\n");
	    return T_ERROR;
	}
	return ((float)( ((full_tod[i+2] + adjust)<<16) + q))/128.0;
//    } else {
//	log_printf(LOG_VERBOSE, "%02X %02X array[%02X%02X] i(%d)\n", sac, sic, full_tod[i], full_tod[i+1],i);
    }
    return T_ERROR;
};

#ifndef CLIENT_RRD
void update_calculations(struct datablock_plot dbp) {
    return;
}    
#endif
