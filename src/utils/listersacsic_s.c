/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2022 Diego Torres <diego dot torres at gmail dot com>

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

/* Imprime "sac sic" por cada datablock cuyo primer registro trae SAC/SIC
 * (mismo criterio que filtersacsic_s). Con -c, en vez del listado, una
 * línea por sensor con el número de datablocks, en el formato de
 * "sort | uniq -c" (que es lo que se hacía siempre con el listado) pero
 * ordenada por SAC y SIC numéricamente. */

// datablocks por sensor (sac<<8 | sic), para -c
static long long counts[65536];

int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0, i;
    int count_mode = 0;
    gs_writer w;
    gps_stream gs;
    unsigned char *db;
    size_t db_bytes;
    // "0".."255" precalculados: una línea por datablock con printf() era la
    // mayor parte del coste
    char num[256][4];
    unsigned char num_len[256];

    if ( argc == 6 && !strcmp(argv[1], "-c") ) {
	count_mode = 1;
	argv++;
	argc--;
    }
    if( argc != 5) {
	printf("listersacsic_s%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("listersacsic_s%s [-c] in_filename.gps headerbytes prebytes postbytes\n", ARCH);
	printf("lists sac/sic pairs found in file (one line per datablock)\n");
	printf("-c: one line per sensor, \"count sac sic\" (like sort | uniq -c, numeric order)\n");
	exit(EXIT_SUCCESS);
    }

    header_bytes = atoi(argv[2]);
    pre_bytes = atoi(argv[3]);
    post_bytes = atoi(argv[4]);

    for ( i = 0; i < 256; i++ )
	num_len[i] = snprintf(num[i], sizeof(num[i]), "%d", i);

    gs_writer_open(&w, "-");
    gs_reader_open(&gs, argv[1], header_bytes, pre_bytes, post_bytes, &w, 0);

    while ( NULL != (db = gs_next(&gs, &db_bytes)) ) {
	unsigned char db_sac = 0, db_sic = 0;
	if ( DEBUG ) print_bytes(db, db_bytes);
	if ( gs_get_sacsic(db, db_bytes, &db_sac, &db_sic) ) {
	    unsigned char line[8];
	    if ( count_mode ) {
		counts[(db_sac << 8) | db_sic]++;
		continue;
	    }
	    size_t n = 0;
	    memcpy(line, num[db_sac], num_len[db_sac]); n += num_len[db_sac];
	    line[n++] = ' ';
	    memcpy(line + n, num[db_sic], num_len[db_sic]); n += num_len[db_sic];
	    line[n++] = '\n';
	    gs_emit(&w, line, n);
	}
    }

    if ( count_mode ) {
	char line[64];
	for ( i = 0; i < 65536; i++ ) {
	    if ( counts[i] ) {
		int n = snprintf(line, sizeof(line), "%7lld %d %d\n", counts[i], i >> 8, i & 0xff);
		gs_emit(&w, (unsigned char *) line, n);
	    }
	}
    }

    gs_writer_close(&w);
    fprintf(stderr, "readed(%lld)\n", gs.rcount);
    gs_reader_close(&gs);
    exit(EXIT_SUCCESS);
}
