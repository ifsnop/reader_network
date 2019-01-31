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

int main(int argc, char *argv[]) {

    struct datablock_plot dbp;
    struct ip_mreq mreq;
    struct sockaddr_in addr;
//    struct hostent * pHostInfo;
//    long nHostAddress;
    int yes = 1, s, dbplen;
    socklen_t addrlen;
    bool forced_exit = false;

    startup();
    printf("client_%s" COPYRIGHT_NOTICE, ARCH, VERSION);

//    pHostInfo = gethostbyname("localost");
//    memcpy(&nHostAddress, pHostInfo->h_addr, pHostInfo->h_length);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PLOTS_PORT);
    if ( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	log_printf(LOG_ERROR, "socket %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0 ) {
        log_printf(LOG_ERROR, "ERROR setsockopt: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ( bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	log_printf(LOG_ERROR, "bind %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }
#if defined(__linux)
    mreq.imr_interface.s_addr = inet_addr("127.0.0.1"); //nHostAddress;
#endif
#if defined(__sun)
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
#endif
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_PLOTS_GROUP);
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
	log_printf(LOG_ERROR, "setsocktperror\n");
	exit(EXIT_FAILURE);
    }

    while (!forced_exit) {
	char *sac_s=0, *sic_l=0;
	dbplen = sizeof(dbp);
	addrlen = sizeof(addr);
	float diff = 0.0;

	if (recvfrom(s, &dbp, dbplen, 0, (struct sockaddr *) &addr, &addrlen) < 0) {
	    log_printf(LOG_ERROR, "recvfrom\n");
	    exit(EXIT_FAILURE);
	}
	if (dbp.cat == CAT_255) {
	    log_printf(LOG_ERROR, "fin de fichero\n");
	    forced_exit = true;
	}
	if ( (dbp.cat == CAT_01) && (dbp.available & IS_TYPE)/* && (dbp.available & IS_TOD)*/ ) {
	    char *hora1 = NULL,*hora2 = NULL;

	    if (dbp.available & IS_TOD) 
		hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) {
		    diff -= 86400;
		}
	    }
	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s%s%s]%s [AZI %03.3f]"
		" [DST %03.3f] [MODEA %04o%s%s%s] [FL%03d%s%s] r%d [%s] (%5.3f) [%s] (%3.4f)\n", 
                dbp.id, /* IFSNOP MOD */
		sac_s , sic_l, 
		(dbp.type == NO_DETECTION) ? "NODET" : "",
		(dbp.type & TYPE_C1_PSR) ? "PSR" : "", 
		(dbp.type & TYPE_C1_SSR) ? "SSR" : "",
		(dbp.type & TYPE_C1_CMB) ? "CMB" : "",
		(dbp.type & TYPE_C1_FIXED_TRANSPONDER) ? "TRN" : "",
		(dbp.flag_test == 1) ? "T" : "",
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
		diff);

	    if (dbp.available & IS_TOD)
	        mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
    		mem_free(sac_s);
		mem_free(sic_l);
	    }

	} else if (dbp.cat == CAT_02 ) {
	    char *hora1, *hora2;

	    hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }

	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) {
		    diff -= 86400;
		}
	    }

	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s%s%s%s] [%s] [%s] (%3.4f)\n", dbp.id,
		sac_s, sic_l,
		(dbp.type == NO_DETECTION) ? "unk" : "",
		(dbp.type == TYPE_C2_NORTH_MARKER) ? "NORTE" : "", 
		(dbp.type == TYPE_C2_SOUTH_MARKER) ? "SUR" : "", 
		(dbp.type == TYPE_C2_SECTOR_CROSSING) ? "SECTOR" : "", 
		(dbp.type == TYPE_C2_START_BLIND_ZONE_FILTERING) ? "START_BLIND" : "", 
		(dbp.type == TYPE_C2_STOP_BLIND_ZONE_FILTERING) ? "STOP_BLIND" : "", 
		hora1, hora2,
		(dbp.available & IS_TOD) ? diff: 0.0);
    	    mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
    		mem_free(sac_s);
		mem_free(sic_l);
	    }
	} else if (dbp.cat == CAT_10 ) {
	    char *hora1, *hora2;

	    hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) {
		    diff -= 86400;
		}
	    }

	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s%s] [%s%s%s%s%s%s%s%s%s%s] [AZI %03.3f] [DST %03.3f] [%s] [%s] (%3.4f)\n", dbp.id,
		sac_s, sic_l,
		(dbp.type == TYPE_C10_TARGET_REPORT) ? "PLOT" : "",
		(dbp.type == TYPE_C10_START_UPDATE_CYCLE) ? "SUC" : "",
		(dbp.type == TYPE_C10_PERIODIC_STATUS) ? "PERIODIC" : "",
		(dbp.type == TYPE_C10_EVENT_STATUS) ? "EVENT" : "",
		(dbp.plot_type == NO_DETECTION) ? "NODETEC" : "",
		(dbp.plot_type == TYPE_C10_PLOT_SSR_MULTI) ? "SSRM" : "",
		(dbp.plot_type == TYPE_C10_PLOT_SSRS_MULTI) ? "SMMS" : "",
		(dbp.plot_type == TYPE_C10_PLOT_ADSB) ? "ADS" : "",
		(dbp.plot_type == TYPE_C10_PLOT_PSR) ? "SMR" : "",
		(dbp.plot_type == TYPE_C10_PLOT_MAGNETIC) ? "MAG" : "",
		(dbp.plot_type == TYPE_C10_PLOT_HF_MULTI) ? "HFM" : "",
		(dbp.plot_type == TYPE_C10_PLOT_NOT_DEFINED) ? "und" : "",
		(dbp.plot_type == TYPE_C10_PLOT_OTHER) ? "OTHER" : "",
		(dbp.type == TYPE_C10_TARGET_REPORT) ? ((dbp.source == SOURCE_CHANNEL1) ? " CH1" : " CH2") : "",
		(dbp.available & IS_MEASURED_POLAR) ? dbp.theta : 0.0,
		(dbp.available & IS_MEASURED_POLAR) ? dbp.rho : 0.0,
		hora1, hora2,
		(dbp.available & IS_TOD) ? diff : 0.0);
	    mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
		mem_free(sac_s);
		mem_free(sic_l);
	    }
	} else if (dbp.cat == CAT_19 ) {
	    char *hora1, *hora2;

	    hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) {
		    diff -= 86400;
		}
	    }

	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s] [%s] [%s] (%3.4f)\n", dbp.id,
		sac_s, sic_l,
		(dbp.type == TYPE_C19_START_UPDATE_CYCLE) ? "UPDATE" : "",
		(dbp.type == TYPE_C19_PERIODIC_STATUS) ? "PERIODIC" : "",
		(dbp.type == TYPE_C19_EVENT_STATUS) ? "EVENT" : "",
		hora1, hora2,
		(dbp.available & IS_TOD) ? diff : 0.0);
	    mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
		mem_free(sac_s);
		mem_free(sic_l);
	    }
	} else if (dbp.cat == CAT_20 ) {
	    char *hora1, *hora2;

	    hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) {
		    diff -= 86400;
		}
	    }

	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s%s%s%s%s%s%s%s%s%s%s%s] [X %03.3f] [Y %03.3f] [%s] [%s] (%3.4f)\n", dbp.id,
		sac_s, sic_l, 
		(dbp.type == TYPE_C20_NONMODESMLAT) ? "SSR_MLAT" : "",
		(dbp.type == TYPE_C20_MODESMLAT) ? "MODES_MLAT" : "",
		(dbp.type == TYPE_C20_HFMLAT) ? "HF_MLAT" : "",
		(dbp.type == TYPE_C20_VDLMLAT) ? "VDL_MAT" : "",
		(dbp.type == TYPE_C20_UATMLAT) ? "UAT_MAT" : "",
		(dbp.type == TYPE_C20_DMEMLAT) ? "DME_MAT" : "",
		(dbp.type == TYPE_C20_OTHERMLAT) ? "OTHER_MAT" : "",
		(dbp.type == TYPE_C20_RAB) ? " fixed" : "",
		(dbp.type == TYPE_C20_SPI) ? " spi" : "",
		(dbp.type == TYPE_C20_CHN) ? " CHN1" : "CHN2", 
		(dbp.type == TYPE_C20_GBS) ? " GBS" : "",
		(dbp.type == TYPE_C20_CRT) ? " CRT" : "",
		(dbp.type == TYPE_C20_SIM) ? " SIM" : "",
		(dbp.type == TYPE_C20_CRT) ? " TEST" : "",
		(dbp.available & IS_MEASURED_CARTE) ? dbp.x : 0.0,
		(dbp.available & IS_MEASURED_CARTE) ? dbp.y : 0.0,
		hora1, hora2,
		(dbp.available & IS_TOD) ? diff : 0.0);
	    mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
		mem_free(sac_s);
		mem_free(sic_l);
	    }
	} else if (dbp.cat == CAT_21 ) {
	    char *hora1, *hora2;

	    hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) {
		    diff -= 86400;
		}
	    }

	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s%s] [%s] [%s] (%3.4f)\n", dbp.id,
		sac_s, sic_l,
		(dbp.flag_test == T_YES) ? "TEST" : "",
		(dbp.flag_ground == T_YES) ? "GROUND" : "",
		(dbp.flag_sim == T_YES) ? "SIM" : "",
		(dbp.flag_fixed == T_YES) ? "FIXED" : "",
		hora1, hora2,
		(dbp.available & IS_TOD) ? diff : 0.0);
	    mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
		mem_free(sac_s);
		mem_free(sic_l);
	    }
	} else if (dbp.cat == CAT_34 ) {
	    char *hora1, *hora2;

	    hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) { // añadido para solucionar un bug v0.63
		    diff -= 86400;
		}
	    }

	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s%s] [%s] [%s] (%3.4f)\n", dbp.id,
		sac_s, sic_l,
		(dbp.type == TYPE_C34_NORTH_MARKER) ? "NORTE" : "",
		(dbp.type == TYPE_C34_SECTOR_CROSSING) ? "SECTOR" : "",
		(dbp.type == TYPE_C34_GEOGRAPHICAL_FILTERING) ? "FILTER" : "",
		(dbp.type == TYPE_C34_JAMMING_STROBE) ? "JAMM" : "",
		hora1, hora2,
		(dbp.available & IS_TOD) ? diff : 0.0);
	    mem_free(hora1);
	    mem_free(hora2);
	    if (dbp.available & IS_SACSIC) {
		mem_free(sac_s);
		mem_free(sic_l);
	    }
	} else if (dbp.cat == CAT_48 ) {
	    char *hora1, *hora2;

	    hora1 = parse_hora(dbp.tod);
	    hora2 = parse_hora(dbp.tod_stamp);
	    if (dbp.available & IS_SACSIC) {
		sac_s = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SAC_SHORT);
		sic_l = ast_get_SACSIC((unsigned char *) &dbp.sac, (unsigned char *) &dbp.sic, GET_SIC_LONG);
	    }
	    if (dbp.available & IS_TOD) {
		diff = dbp.tod_stamp - dbp.tod;
		if (diff <= -86000) { // cuando tod esta en el dia anterior y tod_stamp en el siguiente, la resta es negativa
		    diff += 86400;    // le sumamos un dia entero para cuadrar el calculo
		} else if (diff >=(86400-512)) {
		    diff -= 86400;
		}
	    }

	    log_printf(LOG_VERBOSE, "%ld [%s/%s] [%s%s%s%s%s%s%s%s%s] [AZI %03.3f] [DST %03.3f] [AID %s] [MS%06X] [%s] [%s] (%3.4f)\n", dbp.id,
		sac_s, sic_l, 
		(dbp.type == NO_DETECTION) ? "NODET" : "",
		(dbp.type & TYPE_C48_PSR) ? "PSR" : "",
		(dbp.type & TYPE_C48_SSR) ? "SSR" : "",
		(dbp.type & TYPE_C48_CMB) ? "CMB" : "",
		(dbp.type & TYPE_C48_SSRSGEN) ? "SSRSGEN" : "",
		(dbp.type & TYPE_C48_SSRSROL) ? "SSRSROL" : "",
		(dbp.type & TYPE_C48_CMBSGEN) ? "CMBSGEN" : "",
		(dbp.type & TYPE_C48_CMBSROL) ? "CMBSROL" : "",
		(dbp.type & TYPE_C48_FIXED_TRANSPONDER) ? "TRN" : "",
		(dbp.available & IS_MEASURED_POLAR) ? dbp.theta : 0.0,
		(dbp.available & IS_MEASURED_POLAR) ? dbp.rho : 0.0,
		(dbp.available & IS_AIRCRAFT_ID) ? dbp.aircraft_id : (unsigned char *) "",
		(dbp.available & IS_MODES_ADDRESS) ? dbp.modes_address : 0,
		hora1, hora2,
		(dbp.available & IS_TOD) ? diff : 0.0);
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

    exit(EXIT_SUCCESS);
}

