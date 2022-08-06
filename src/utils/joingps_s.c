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

void print_separator() {
    fprintf(stderr, "==============================================\n");
    return;
}

void print_bytes(unsigned char* p, ssize_t count) {
    ssize_t i = 0;
    fprintf(stderr, "%zu [", count);
    for(i = 0; i < count; i++)
	if ( (i + 1) < count )
	    fprintf(stderr, "%02X ", p[i]);
	else
	    fprintf(stderr, "%02X", p[i]);
    fprintf(stderr, "]\n");
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
	    fprintf(stderr, "error reading bytes (%s)\n", strerror(errno)); exit(1);
	}
    }
    // if ( DEBUG ) print_bytes(p, rcount);
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
    // if ( DEBUG ) print_bytes(p, count);
    return wcount;
}

float get_ts(unsigned char *p, ssize_t size) {

    float ts = ( (p[size - 4] <<16 ) +
	         (p[size - 3] << 8 ) +
	         (p[size - 2]) ) / 128.0;
    return ts;
}

int main(int argc, char *argv[]) {
    FILE *fdin1 = NULL, *fdin2 = NULL, *fdout = NULL;
    ssize_t lendb1 = -1, lendb2 = -1;
    unsigned char *ptr1 = NULL, *ptr2 = NULL;
    float ts1 = -1, ts2 = -1;
    ssize_t rcount = 0, count = 0, wcount = 0;
    ssize_t BUFFER_SIZE = 1024*1024;

    if(argc!=4) {
	printf("joingps_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("joingps_%s in_filename_1.gps in_filename_2.gps out_filename.gps\n", ARCH);
	printf("filename_1.gps and out_filename.gps can be stdin & stdout\n\n");
	exit(EXIT_SUCCESS);
    }

    if ( !strcmp(argv[1], "-") ) {
	if ( NULL == (fdin1 = fdopen(dup(fileno(stdin)), "rb")) ) {
	    fprintf(stderr, "error input file 1 (%s)\n", strerror(errno)); exit(1);
	}
    } else {
	if ( NULL == (fdin1 = fopen(argv[1], "rb")) ) {
	    fprintf(stderr, "error input file 1 (%s)\n", strerror(errno)); exit(1);
	}
    }
    if ( !strcmp(argv[2], "-") ) {
	if ( NULL == (fdin2 = fdopen(dup(fileno(stdin)), "rb")) ) {
	    fprintf(stderr, "error input file 2 (%s)\n", strerror(errno)); exit(1);
	}
    } else {
	if ( NULL == (fdin2 = fopen(argv[2], "rb")) ) {
	    fprintf(stderr, "error input file 2 (%s)\n", strerror(errno)); exit(1);
	}
    }
    if ( !strcmp(argv[3], "-") ) {
	fdout = stdout;
    } else {
	if ( NULL == (fdout = fopen(argv[3], "wb")) ) {
	    fprintf(stderr, "error output file (%s)\n", strerror(errno)); exit(1);
	}
    }

    ptr1 = calloc(1, BUFFER_SIZE);
    ptr2 = calloc(1, BUFFER_SIZE);

    // CABECERA FICHERO 2
    if ( DEBUG ) printf("leyendo cabecera fichero 1\n");
    if ( 2200 != (count = read_bytes(ptr2, 2200, fdin2)) ) {
	fprintf(stderr, "error reading header header_bytes(%d) count(%zu) from file 1\n", 2200, count);
	exit(EXIT_FAILURE);
    }
    rcount += 2200; // la diferencia entre leidos y escritos debe ser de 2200, porque no escribimos la
    // CABECERA FICHERO 1
    // ahora si leemos la cabecera del primer fichero para copiar al segundo
    if ( DEBUG ) printf("leyendo cabecera fichero 2\n");
    if ( 2200 != (count = read_bytes(ptr1, 2200, fdin1)) ) {
	fprintf(stderr, "error reading header header_bytes(%d) count(%zu) from file 1\n", 2200, count);
	exit(EXIT_FAILURE);
    }
    rcount += 2200;
    write_bytes(ptr1, 2200, fdout); wcount += 2200;
    // if ( DEBUG ) print_bytes(ptr1, 2200);

    // COMIENZA

    // LEEMOS FICHERO 1
    if ( DEBUG ) printf("leyendo fichero 1\n");
    if ( 3 != (lendb1 = read_bytes(ptr1, 3, fdin1)) ) {
	lendb1 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 1\n");
    }
    if ( lendb1 > 0 ) {
	lendb1 = ((ptr1[1] & 0xff) << 8 ) + (ptr1[2] & 0xff);
	if ( (lendb1 - 3 + 10) != read_bytes(ptr1 + 3, lendb1 - 3 + 10, fdin1) ) {
	    lendb1 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 1 - db a medio leer\n");
	};
	if ( lendb1 > 0 ) {
	    rcount += lendb1 + 10;
	    ts1 = get_ts(ptr1, lendb1 + 10);
	    if ( DEBUG ) printf("ts1: %f\n", ts1);
	}
    }

    // LEEMOS FICHERO 2
    if ( DEBUG ) printf("leyendo fichero 2\n");
    if ( 3 != (lendb2 = read_bytes(ptr2, 3, fdin2)) ) {
	lendb2 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 2\n");
    }
    if ( lendb2 > 0 ) {
	lendb2 = ((ptr2[1] & 0xff) << 8 ) + (ptr2[2] & 0xff);
	if ( (lendb2 - 3 + 10) != read_bytes(ptr2 + 3, lendb2 - 3 + 10, fdin2) ) {
	    lendb2 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 2 - db a medio leer\n");
	};
	if ( lendb2 > 0 ) {
	    rcount += lendb2 + 10;
	    ts2 = get_ts(ptr2, lendb2 + 10);
	    if ( DEBUG ) printf("ts2: %f\n", ts2);
	}
    }


    for (;;) {
	if ( lendb1 == -1 || lendb2 == -1 )
	    break;
	if ( ts1 < ts2 ) {
	    if ( DEBUG ) printf("escribimos fichero 1\n");
	    // ESCRIBIMOS DE FICHERO 1
	    write_bytes(ptr1, lendb1 + 10, fdout); wcount += lendb1 + 10;
	    // LEEMOS FICHERO 1
	    if ( DEBUG ) printf("leemos fichero 1\n");
	    if ( 3 != (lendb1 = read_bytes(ptr1, 3, fdin1)) ) {
		lendb1 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 1\n");
	    }
	    if ( lendb1 > 0 ) {
		lendb1 = ((ptr1[1] & 0xff) << 8 ) + (ptr1[2] & 0xff);
		if ( (lendb1 - 3 + 10) != read_bytes(ptr1 + 3, lendb1 - 3 + 10, fdin1) ) {
		    lendb1 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 1 - db a medio leer\n");
		};
		if ( lendb1 > 0 ) {
		    rcount += lendb1 + 10;
		    ts1 = get_ts(ptr1, lendb1 + 10);
		    if ( DEBUG ) printf("ts1: %f\n", ts1);
		}
	    }
	} else {
	    if ( DEBUG ) printf("escribimos fichero 2\n");
	    // ESCRIBIMOS DE FICHERO 2
	    write_bytes(ptr2, lendb2 + 10, fdout); wcount += lendb2 + 10;
	    // LEEMOS FICHERO 2
	    if ( DEBUG ) printf("leemos fichero 2\n");
	    if ( 3 != (lendb2 = read_bytes(ptr2, 3, fdin2)) ) {
		lendb2 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 2\n");
	    }
	    if ( lendb2 > 0 ) {
		lendb2 = ((ptr2[1] & 0xff) << 8 ) + (ptr2[2] & 0xff);
		if ( (lendb2 - 3 + 10) != read_bytes(ptr2 + 3, lendb2 - 3 + 10, fdin2) ) {
		    lendb2 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 2 - db a medio leer\n");
		};
		if ( lendb2 > 0 ) {
		    rcount += lendb2 + 10;
		    ts2 = get_ts(ptr2, lendb2 + 10);
		    if ( DEBUG ) printf("ts2: %f\n", ts2);
		}
	    }
	}
    }

    if ( DEBUG ) printf("SALIENDO BUCLE\n");

    // VOLCAMOS EL RESTO DEL FICHERO 1
    while ( lendb1 > 0 ) {
	if ( DEBUG ) printf("0)escribimos resto fichero 1\n");
	write_bytes(ptr1, lendb1 + 10, fdout); wcount += lendb1 + 10;
	// LEEMOS FICHERO 1
	if ( DEBUG ) printf("seguimos leyendo fichero 1\n");
	if ( 3 != (lendb1 = read_bytes(ptr1, 3, fdin1)) ) {
	    lendb1 = -1; if ( DEBUG ) fprintf(stderr, "1)se acabo fichero 1 (%ld)\n", lendb1);
	}
	if ( lendb1 > 0 ) {
	    lendb1 = ((ptr1[1] & 0xff) << 8 ) + (ptr1[2] & 0xff);
	    if ( (lendb1 - 3 + 10) != read_bytes(ptr1 + 3, lendb1 - 3 + 10, fdin1) ) {
		lendb1 = -1; if ( DEBUG ) fprintf(stderr, "2)se acabo fichero 1 (%ld) - db a medio leer\n", lendb1);
	    };
	    if ( lendb1 > 0 ) {
		rcount += lendb1 + 10;
		ts1 = get_ts(ptr1, lendb1 + 10);
		if ( DEBUG ) printf("ts1: %f\n", ts1);
	    }
	}
    }

    // VOLCAMOS EL RESTO DEL FICHERO 2
    while ( lendb2 > 0 ) {
	if ( DEBUG ) printf("escribimos resto fichero 2\n");
        write_bytes(ptr2, lendb2 + 10, fdout); wcount += lendb2 + 10;
	if ( DEBUG ) printf("seguimos leyendo fichero 2\n");
        // LEEMOS FICHERO 2
        if ( 3!= (lendb2 = read_bytes(ptr2, 3, fdin2)) ) {
	    lendb2 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 2\n");
	}
	if ( lendb2 > 0 ) {
	    lendb2 = ((ptr2[1] & 0xff) << 8 ) + (ptr2[2] & 0xff);
	    if ( (lendb2 - 3 + 10) != read_bytes(ptr2 + 3, lendb2 - 3 + 10, fdin2) ) {
		lendb2 = -1; if ( DEBUG ) fprintf(stderr, "se acabo fichero 2 - db a medio leer\n");
	    };
	    if ( lendb2 > 0 ) {
		rcount += lendb2 + 10;
		ts2 = get_ts(ptr2, lendb2 + 10);
		if ( DEBUG ) printf("ts2: %f\n", ts2);
	    }
	}
    }

    fprintf(stderr, "readed(%zu)/written(%zu)\n", rcount, wcount);

    free(ptr1);
    free(ptr2);
    if (fclose(fdout) == -1) {
	fprintf(stderr, "error close fdout\n"); exit(1);
    }
    if (fclose(fdin2) == -1) {
	fprintf(stderr, "error close file 2\n"); exit(1);
    }
    if (fclose(fdin1) == -1) {
	fprintf(stderr, "error close file 1\n"); exit(1);
    }

    exit(EXIT_SUCCESS);
}
