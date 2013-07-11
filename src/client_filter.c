/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2013 Diego Torres <diego dot torres at gmail dot com>

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

int main(int argc, char *argv[]) {

    struct datablock_plot dbp;
    struct ip_mreq mreq;
    struct sockaddr_in addr;
    float azi_start = -1.0;
    float azi_end = -1.0;
    float rng_start = -1.0;
    float rng_end = -1.0;
//    struct hostent * pHostInfo;
//    long nHostAddress;
    int yes = 1, s, dbplen, addrlen;

    startup();
    
    log_printf(LOG_NORMAL, "init... %d\n", argc);
    if (argc>1) {
        log_printf(LOG_NORMAL, "azimuth start: %3.3f\n", azi_start = atof(argv[1]));
	log_printf(LOG_NORMAL, "azimuth   end: %3.3f\n", azi_end = atof(argv[2]));
    }
    if (argc>3) {
        log_printf(LOG_NORMAL, "range   start: %3.3f\n", rng_start = atof(argv[3]));
	log_printf(LOG_NORMAL, "range     end: %3.3f\n", rng_end = atof(argv[4]));
    }
    
    
    
//    pHostInfo = gethostbyname("localost");
//    memcpy(&nHostAddress, pHostInfo->h_addr, pHostInfo->h_length);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PLOTS_PORT);
    if ( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	log_printf(LOG_ERROR, "socket %s\n", strerror(errno));
	exit(1);
    }
    
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if ( bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	log_printf(LOG_ERROR, "bind %s\n", strerror(errno));
	exit(1);
    }
				
    mreq.imr_interface.s_addr = inet_addr("127.0.0.1"); //nHostAddress;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_PLOTS_GROUP);
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
	log_printf(LOG_ERROR, "setsocktperror\n");
	exit(1);
    }

    while (1) {
	char *sac_s=0, *sic_l=0;
	dbplen = sizeof(dbp);
	addrlen = sizeof(addr);

	if (recvfrom(s, &dbp, dbplen, 0, (struct sockaddr *) &addr, &addrlen) < 0) {
	    log_printf(LOG_ERROR, "recvfrom\n");
	    exit(1);
	}

	if (dbp.cat == CAT_02 ) {
	    log_printf(LOG_VERBOSE, "--------------------------------\n");
	}
        
	if ( (dbp.cat == CAT_01) && (dbp.available & IS_TYPE)/* && (dbp.available & IS_TOD)*/ ) {
	    char *hora1,*hora2;
	    if (azi_start != -1.0) {
		if (! ( (dbp.available & IS_MEASURED_POLAR) && 
			(dbp.theta >= azi_start ) &&
			(dbp.theta <= azi_end)) ) {
		    continue;	
		}
	    }
	    if (rng_start != -1.0) {
		if (! ( (dbp.available & IS_MEASURED_POLAR) && 
			(dbp.rho >= rng_start ) &&
			(dbp.rho <= rng_end)) ) {
		    continue;	
		}
	    }

	    if (dbp.available & IS_TOD) 
		hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    log_printf(LOG_VERBOSE, "[%s/%s] [PLOT %s%s%s%s%s] [AZI %3.3f]"
		" [DST %03.3f] [MODEA %04o%s%s%s] [FL%03d%s%s] r%d [%s] (%5.3f) [%s] (%3.4f)\n",
		sac_s , sic_l, 
		(dbp.type == NO_DETECTION) ? "unk" : "",
		(dbp.type & TYPE_C1_PSR) ? "PSR" : "", 
		(dbp.type & TYPE_C1_SSR) ? "SSR" : "",
		(dbp.type & TYPE_C1_CMB) ? "CMB" : "",
		(dbp.type & FROM_C1_FIXED_TRANSPONDER) ? "-TRN" : "",
		(dbp.available & IS_MEASURED_POLAR) ? dbp.theta : 0.0,
		(dbp.available & IS_MEASURED_POLAR) ? dbp.rho : 0.0,
		(dbp.available & IS_MODEA) ? dbp.modea : 0,
		(dbp.modea_status & STATUS_MODEA_GARBLED) ? "G" : "",
		(dbp.modea_status & STATUS_MODEA_NOTVALIDATED) ? "I" : "",
		(dbp.modea_status & STATUS_MODEA_SMOOTHED) ? "S" : "",
		(dbp.available & IS_MODEC) ? dbp.modec : -1,
		(dbp.modec_status & STATUS_MODEC_GARBLED) ? "G" : "",
		(dbp.modec_status & STATUS_MODEC_NOTVALIDATED) ? "I" : "",
		(dbp.radar_responses),
		(dbp.available & IS_TOD) ? hora1 : "na",
		(dbp.available & IS_TRUNCATED_TOD) ? dbp.truncated_tod : 0.0,
		hora2,
		(dbp.available & IS_TOD) ? dbp.tod_stamp - dbp.tod : 0.0);

	    if (dbp.available & IS_TOD)
	        mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
    		mem_free(sac_s);
		mem_free(sic_l);
	    }

	}
    }
    log_printf(LOG_NORMAL, "end...\n");
//    log_flush();

    exit(0);
}

