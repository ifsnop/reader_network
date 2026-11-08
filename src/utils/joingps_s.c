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

#define JG_HEADER_BYTES 2200
#define JG_POST_BYTES 10

/* hora gps del registro en 1/128 s (bytes 6-8 del trailer). Comparar en
 * enteros equivale a la comparación en float de antes: < 2^24 es exacto */
static long ts128(const unsigned char *db, size_t db_bytes) {
    const unsigned char *post = db + db_bytes;
    return (post[JG_POST_BYTES - 4] << 16) + (post[JG_POST_BYTES - 3] << 8) + post[JG_POST_BYTES - 2];
}

/* Mezcla dos ficheros .gps por la hora gps de cada registro. La cabecera de
 * la salida es la del fichero 1; con la misma hora va antes el registro del
 * fichero 2. Cuando uno se acaba, se vuelca el resto del otro. */
int main(int argc, char *argv[]) {
    gs_writer w;
    gps_stream gs1, gs2;
    unsigned char *db1, *db2;
    size_t n1 = 0, n2 = 0;

    if(argc!=4) {
	printf("joingps_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("joingps_%s in_filename_1.gps in_filename_2.gps out_filename.gps\n", ARCH);
	printf("filename_1.gps and out_filename.gps can be stdin & stdout\n\n");
	exit(EXIT_SUCCESS);
    }

    gs_writer_open(&w, argv[3]);
    // la cabecera del fichero 2 se lee y se descarta; la del 1 se copia
    gs_reader_open(&gs2, argv[2], JG_HEADER_BYTES, 0, JG_POST_BYTES, &w, 0);
    gs_reader_open(&gs1, argv[1], JG_HEADER_BYTES, 0, JG_POST_BYTES, &w, 1);

    db1 = gs_next(&gs1, &n1);
    db2 = gs_next(&gs2, &n2);
    while ( NULL != db1 && NULL != db2 ) {
	if ( ts128(db1, n1) < ts128(db2, n2) ) {
	    if ( DEBUG ) printf("escribimos fichero 1\n");
	    gs_keep(&gs1);
	    db1 = gs_next(&gs1, &n1);
	} else {
	    if ( DEBUG ) printf("escribimos fichero 2\n");
	    gs_keep(&gs2);
	    db2 = gs_next(&gs2, &n2);
	}
    }
    // volcamos el resto del fichero que no se ha acabado
    while ( NULL != db1 ) {
	gs_keep(&gs1);
	db1 = gs_next(&gs1, &n1);
    }
    while ( NULL != db2 ) {
	gs_keep(&gs2);
	db2 = gs_next(&gs2, &n2);
    }

    gs_flush(&gs1);
    gs_flush(&gs2);
    gs_flush_out(&w);
    fprintf(stderr, "readed(%lld)/written(%lld)\n", gs1.rcount + gs2.rcount, w.wcount);
    gs_writer_close(&w);
    gs_reader_close(&gs2);
    gs_reader_close(&gs1);
    exit(EXIT_SUCCESS);
}
