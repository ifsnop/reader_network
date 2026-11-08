/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2020 Diego Torres <diego dot torres at gmail dot com>

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

/* incluir el primero: define _FILE_OFFSET_BITS antes de los includes del
 * sistema */
#include "gps_stream.h"
#include "defines.h"

#define DEBUG 0

/* Devuelve 1 y el SAC/SIC del primer registro del datablock si este trae
 * I0xx/010 (bit 0x80 del primer byte del FSPEC; es el FRN 1 en las
 * categorías habituales). El FSPEC y el SAC/SIC tienen que caber en el
 * datablock. */
static int get_sacsic(const unsigned char *db, size_t db_bytes, unsigned char *sac, unsigned char *sic) {
    size_t o = 3;
    if ( db_bytes < 6 || 0 == (db[3] & 0x80) )
	return 0;
    while ( db[o] & 1 ) { // FX: el FSPEC sigue en el byte siguiente
	if ( ++o + 2 >= db_bytes )
	    return 0;
    }
    if ( o + 2 >= db_bytes )
	return 0;
    *sac = db[o + 1];
    *sic = db[o + 2];
    return 1;
}

int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0;
    int i, negative = -1;
    // un bit por sensor (sac<<8 | sic), 8 KB: una sola consulta por
    // registro, sea cual sea el número de parejas
    unsigned char sel[65536 / 8];
    gps_stream gs;
    unsigned char *db;
    size_t db_bytes;

    if( argc < 8 || 0 != (argc - 6) % 2 ) {
	printf("filtersacsic_s%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("filtersacsic_s%s in_filename.gps out_filename.gps headerbytes prebytes postbytes sac sic [sac sic ...]\n", ARCH);
	printf("positive sac sic: include only asterix from sensors with these sac sic\n");
	printf("negative sac sic (-sac -sic): exclude asterix from sensors with these sac sic\n");
	printf("all pairs must have the same sign (\"0 -sic\" is accepted as \"-0 -sic\")\n");
	printf("asterix without sac sic is always excluded\n\n");
	exit(EXIT_SUCCESS);
    }

    header_bytes = atoi(argv[3]);
    pre_bytes = atoi(argv[4]);
    post_bytes = atoi(argv[5]);

    memset(sel, 0, sizeof(sel));
    for ( i = 6; i + 1 < argc; i += 2 ) {
	int neg_sac, neg_sic, neg, key;
	int s = gs_parse_signed(argv[i], 255, &neg_sac);
	int c = gs_parse_signed(argv[i + 1], 255, &neg_sic);
	if ( s < 0 || c < 0 ) {
	    fprintf(stderr, "error in sac sic (%s %s), expected 0..255 or -0..-255\n", argv[i], argv[i + 1]); exit(1);
	}
	// los dos con el mismo signo; "0 -sic" se acepta como "-0 -sic"
	// (antes era la forma de excluir un sensor con SAC 0)
	if ( neg_sac == neg_sic ) {
	    neg = neg_sac;
	} else if ( 0 == s && !neg_sac && neg_sic ) {
	    neg = 1;
	} else {
	    fprintf(stderr, "error in sac sic (%s %s), both must have the same sign\n", argv[i], argv[i + 1]); exit(1);
	}
	if ( -1 == negative ) {
	    negative = neg;
	} else if ( negative != neg ) {
	    fprintf(stderr, "error sac sic pairs must be all positive (include) or all negative (exclude)\n"); exit(1);
	}
	key = (s << 8) | c;
	sel[key >> 3] |= 1 << (key & 7);
    }

    gs_open(&gs, argv[1], argv[2], header_bytes, pre_bytes, post_bytes);

    while ( NULL != (db = gs_next(&gs, &db_bytes)) ) {
	unsigned char db_sac = 0, db_sic = 0;
	if ( DEBUG ) print_bytes(db, db_bytes);
	if ( get_sacsic(db, db_bytes, &db_sac, &db_sic) ) {
	    int key = (db_sac << 8) | db_sic;
	    int in_list = (sel[key >> 3] >> (key & 7)) & 1;
	    if ( DEBUG ) fprintf(stderr, "db_sac %d db_sic %d in_list %d\n", db_sac, db_sic, in_list);
	    // positivas: pasa si está en la lista; negativas: si no está
	    if ( in_list != negative )
		gs_keep(&gs);
	}
	if ( DEBUG ) print_separator();
    }

    gs_close(&gs);
    exit(EXIT_SUCCESS);
}
