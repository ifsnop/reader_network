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

int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0;
    int i, neg = 0, negative = -1;
    // sel[cat] != 0 si la categoría está en la lista: una sola consulta por
    // registro, sea cual sea el número de categorías
    unsigned char sel[256];
    gps_stream gs;
    unsigned char *db;
    size_t db_bytes;

    if( argc < 7 ) {
	printf("filtercat_s%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("filtercat_s%s in_filename.gps out_filename.gps headerbytes prebytes postbytes cat [cat ...]\n", ARCH);
	printf("positive cats: include only asterix with these cats\n");
	printf("negative cats: exclude asterix with these cats\n");
	printf("all cats must have the same sign\n\n");
	exit(EXIT_SUCCESS);
    }

    header_bytes = atoi(argv[3]);
    pre_bytes = atoi(argv[4]);
    post_bytes = atoi(argv[5]);

    memset(sel, 0, sizeof(sel));
    for ( i = 6; i < argc; i++ ) {
	int c = gs_parse_signed(argv[i], 255, &neg);
	if ( c < 0 ) {
	    fprintf(stderr, "error in cat (%s), expected 0..255 or -0..-255\n", argv[i]); exit(1);
	}
	if ( -1 == negative ) {
	    negative = neg;
	} else if ( negative != neg ) {
	    fprintf(stderr, "error cats must be all positive (include) or all negative (exclude)\n"); exit(1);
	}
	sel[c] = 1;
    }

    gs_open(&gs, argv[1], argv[2], header_bytes, pre_bytes, post_bytes);

    while ( NULL != (db = gs_next(&gs, &db_bytes)) ) {
	if ( DEBUG ) print_bytes(db, db_bytes);
	// positivas: pasa si está en la lista; negativas: si no está
	if ( sel[db[0]] != negative ) {
	    if ( DEBUG ) fprintf(stderr, "OK\n");
	    gs_keep(&gs);
	}
	if ( DEBUG ) print_separator();
    }

    gs_close(&gs);
    exit(EXIT_SUCCESS);
}
