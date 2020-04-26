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
	if ( (i + 1) < count )
	    fprintf(stderr, "%02X ", p[i]);
	else
	    fprintf(stderr, "%02X", p[i]);
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
	    // fprintf(stderr, "end of input file\n");
	} else {
	    fprintf(stderr, "error reading header (%s)\n", strerror(errno)); exit(1);
	}
    }
    return rcount;
}

ssize_t write_bytes(unsigned char *p, ssize_t count, FILE * f) {

    if ( 0 == count )
	return 0;

    ssize_t wcount = 0;
    if ( 0 == (wcount = fwrite(p, 1, count, f)) ) {
	fprintf(stderr, "error writing output (%s)", strerror(errno));
	exit(EXIT_FAILURE);
    }
    return wcount;
}

int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0, db_bytes = 0, cat = 0;
    FILE *fin = NULL, *fout = NULL;
    // unsigned int lendb, filesize;

    unsigned char *ptr = NULL, *pre_ptr = NULL, *post_ptr = NULL;
    ssize_t BUFFER_SIZE = 1024*1024;

    ssize_t rcount = 0, count = 0, wcount = 0;

    if( argc != 7 ) {
	printf("filtercat_s%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("filtercat_s%s in_filename.gps out_filename.gps headerbytes prebytes postbytes cat\n", ARCH);
	printf("positive cat: include asterix with this cat\n");
	printf("negative cat: exclude asterix with this cat\n\n");
	exit(EXIT_SUCCESS);
    }

    header_bytes = atoi(argv[3]);
    pre_bytes = atoi(argv[4]);
    post_bytes = atoi(argv[5]);
    cat = atoi(argv[6]);

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
	if ( header_bytes != (count = read_bytes(ptr, header_bytes, fin)) ) {
	    fprintf(stderr, "error reading header header_bytes(%d) count(%lu)\n", header_bytes, count);
	    exit(EXIT_FAILURE);
	}
	write_bytes(ptr, header_bytes, fout);
	wcount += header_bytes;
	rcount += header_bytes;
	if ( DEBUG ) print_bytes(ptr, header_bytes);
	free(ptr);
    }
    if ( DEBUG ) print_separator();

    ptr = calloc(1, BUFFER_SIZE);
    pre_ptr = calloc(1, pre_bytes);
    post_ptr = calloc(1, post_bytes);

    for(;;) {
	if ( 0 != pre_bytes  && (pre_bytes == read_bytes(pre_ptr, pre_bytes, fin)) ) {
	    if ( DEBUG ) print_bytes(pre_ptr, pre_bytes);
	}

	if ( 3 != read_bytes(ptr, 3, fin) ) {
	    break;
	}
	db_bytes = ((ptr[1] & 0xff) <<8) + (ptr[2] & 0xff);
	if ( (db_bytes - 3) != read_bytes(ptr + 3, db_bytes - 3, fin) ) {
	    break;
	}
	if ( DEBUG ) print_bytes(ptr, db_bytes);

	if ( 0 != post_bytes && (post_bytes == read_bytes(post_ptr, post_bytes, fin)) ) {
	    if ( DEBUG ) print_bytes(post_ptr, post_bytes);
	}

	rcount += pre_bytes + db_bytes + post_bytes;

//	fprintf(stderr, "%02X %02X ____ ", -cat, ptr[0]);

       if ( ((0 < cat) && (cat == ptr[0])) ||  // cat positivo
           ((0 > cat) && (-cat != ptr[0])) ) { // cat negativo
	    // fprintf(stderr, "%02X %02X\n", cat, ptr[0]);
	    if ( DEBUG ) fprintf(stderr, "OK\n");
	    write_bytes(pre_ptr, pre_bytes, fout);
	    write_bytes(ptr, db_bytes, fout);
	    write_bytes(post_ptr, post_bytes, fout);
	    wcount += pre_bytes + db_bytes + post_bytes;
	}
	if ( DEBUG ) print_separator();

    }
    fprintf(stderr, "readed(%lu)/written(%lu)\n", rcount, wcount);

    free(ptr);
    free(pre_ptr);
    free(post_ptr);
    if ( fout && (0 != fclose(fout)) ) {
	fprintf(stderr, "error close fout (%s)\n", strerror(errno)); exit(1);
    }
    if ( fin && (0 != fclose(fin)) ) {
	fprintf(stderr, "error close fin (%s)\n", strerror(errno)); exit(1);
    }
    exit(EXIT_SUCCESS);
}
