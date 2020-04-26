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

#include <stdio.h>
/*#include <unistd.h>*/
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "defines.h"

#define DEBUG 0

void print_bytes(unsigned char* p, ssize_t count) {
    fprintf(stderr, "%ld [", count);
    for(ssize_t i = 0; i < count; i++)
	fprintf(stderr, "%02X ", p[i]);
    fprintf(stderr, "]\n");
    return;
}

void print_separator() {
    fprintf(stderr, "==============================================\n");
    return;
}

ssize_t read_bytes(unsigned char *p, ssize_t count, FILE * f) {

    if ( 0 == count )
	return 0;

    ssize_t rcount = 0;
    if ( 0 == (rcount = fread(p, 1, count, f)) ) {
	if ( feof(f) ) {
	    //fprintf(stderr, "end of input file\n");
	} else {
	    fprintf(stderr, "error reading header (%s)\n", strerror(errno)); exit(1);
	}
    }
    return rcount;
}

int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0, db_bytes = 0;
    FILE *fin = NULL, *fout = NULL;
    // unsigned int lendb, filesize;

    unsigned char *ptr = NULL;
    ssize_t BUFFER_SIZE = 1024*1024;

    ssize_t rcount = 0, count = 0, wcount = 0;

    if(argc!=6) {
	printf("cleanast_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("cleanast_%s in_filename.gps out_filename.ast headerbytes prebytes postbytes\n\n", ARCH);
	exit(EXIT_SUCCESS);
    }

    header_bytes = atoi(argv[3]);
    pre_bytes = atoi(argv[4]);
    post_bytes = atoi(argv[5]);

    if ( !strcmp(argv[1], "-") ) {
	if ( NULL == (fin = fdopen(dup(fileno(stdin)), "rb")) ) {
	    fprintf(stderr, "error input file (%s)\n", strerror(errno)); exit(1);
	}
    } else {
	if ( NULL == (fin = fopen(argv[1], "rb")) ) {
	    fprintf(stderr, "error input file (%s)\n", strerror(errno)); exit(1);
	}
    }

    if ( !strcmp(argv[2], "-") ) {
	fout = stdout;
    } else {
	if ( NULL == (fout = fopen(argv[2], "wb")) ) {
	    fprintf(stderr, "error output file (%s)\n", strerror(errno)); exit(1);
	}
    }

    if ( 0 != header_bytes ) {
	ptr = calloc(1, header_bytes);
	if ( header_bytes != (rcount = read_bytes(ptr, header_bytes, fin)) ) {
	    fprintf(stderr, "error reading header header_bytes(%d) rcount(%lu)\n", header_bytes, rcount);
	    exit(EXIT_FAILURE);
	}
	if ( DEBUG ) print_bytes(ptr, header_bytes);
	free(ptr);
    }

    count += header_bytes;
    if ( DEBUG ) print_separator();

    ptr = calloc(1, BUFFER_SIZE);
    for(;;) {
	if ( 0 != pre_bytes  && (pre_bytes == read_bytes(ptr, pre_bytes, fin)) ) {
	    count += pre_bytes;
	    if ( DEBUG ) print_bytes(ptr, pre_bytes);
	}

	if ( 3 != read_bytes(ptr, 3, fin) ) {
	    break;
	}
	db_bytes = ((ptr[1] & 0xff) <<8) + (ptr[2] & 0xff);
	if ( (db_bytes - 3) != read_bytes(ptr + 3, db_bytes - 3, fin) ) {
	    break;
	}
	count += db_bytes;
	wcount += db_bytes;
	if ( DEBUG ) print_bytes(ptr, db_bytes);

	if ( 0 == fwrite(ptr, 1, db_bytes, fout) ) {
	    fprintf(stderr, "error writing output (%s)", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	if ( 0 != post_bytes && (post_bytes == read_bytes(ptr, post_bytes, fin)) ) {
	    count += post_bytes;
	    if ( DEBUG ) print_bytes(ptr, post_bytes);
	}
	if ( DEBUG ) print_separator();
    }
    fprintf(stderr, "readed(%lu)/written(%lu)\n", count, wcount);

    free(ptr);
    if ( fout && (0 != fclose(fout)) ) {
	fprintf(stderr, "error close fout (%s)\n", strerror(errno)); exit(1);
    }
    if ( fin && (0 != fclose(fin)) ) {
	fprintf(stderr, "error close fin (%s)\n", strerror(errno)); exit(1);
    }
    exit(EXIT_SUCCESS);
}
