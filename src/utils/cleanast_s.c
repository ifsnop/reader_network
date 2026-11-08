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

/* Quita el envoltorio gps (cabecera, pre_bytes y post_bytes) y deja solo
 * los datablocks ASTERIX. */
int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0;
    gs_writer w;
    gps_stream gs;
    unsigned char *db;
    size_t db_bytes;

    if(argc!=6) {
	printf("cleanast_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("cleanast_%s in_filename.gps out_filename.ast headerbytes prebytes postbytes\n\n", ARCH);
	exit(EXIT_SUCCESS);
    }

    header_bytes = atoi(argv[3]);
    pre_bytes = atoi(argv[4]);
    post_bytes = atoi(argv[5]);

    gs_writer_open(&w, argv[2]);
    gs_reader_open(&gs, argv[1], header_bytes, pre_bytes, post_bytes, &w, 0); // sin cabecera

    while ( NULL != (db = gs_next(&gs, &db_bytes)) ) {
	if ( DEBUG ) print_bytes(db, db_bytes);
	gs_emit(&w, db, db_bytes);
    }

    fprintf(stderr, "readed(%lld)/written(%lld)\n", gs.rcount, w.wcount);
    gs_reader_close(&gs);
    gs_writer_close(&w);
    exit(EXIT_SUCCESS);
}
