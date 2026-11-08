/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2024 Diego Torres <diego dot torres at gmail dot com>

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

/* ficheros > 2 GiB también en el binario de 32 bits (no-op en 64 bits y
 * en libc antiguas que no lo conocen) */
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#define __USE_XOPEN
#define _GNU_SOURCE
#include <time.h>
#include "defines.h"

#define DEBUG 0

/* Se lee la entrada en bloques de este tamaño y los registros se parsean
 * dentro del propio buffer (un registro ocupa como máximo
 * pre_bytes + 65535 + post_bytes). Los registros aceptados consecutivos se
 * escriben con un solo write() directamente desde este buffer. */
#define FT_BUFFER_SIZE (4*1024*1024)

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

void print_separator() {
    fprintf(stderr, "==============================================\n");
    return;
}

/* lee hasta count bytes (menos solo en EOF) */
static size_t read_full(int fd, unsigned char *p, size_t count) {
    size_t got = 0;
    while ( got < count ) {
	ssize_t r = read(fd, p + got, count - got);
	if ( 0 == r )
	    break;
	if ( r < 0 ) {
	    if ( EINTR == errno )
		continue;
	    fprintf(stderr, "error reading input (%s)\n", strerror(errno)); exit(1);
	}
	got += r;
    }
    return got;
}

static void write_full(int fd, const unsigned char *p, size_t count) {
    while ( count > 0 ) {
	ssize_t w = write(fd, p, count);
	if ( w < 0 ) {
	    if ( EINTR == errno )
		continue;
	    fprintf(stderr, "error writing output (%s)", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	p += w;
	count -= w;
    }
}


int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0;
    int fin = -1, fout = -1, is_regular = 0;
    struct stat st;

    unsigned char *buf = NULL;
    size_t len = 0, pos = 0;          // bytes válidos en buf / siguiente registro
    size_t run_start = 0, run_len = 0; // tramo de registros aceptados pendiente de escribir
    int eof = 0;

    long long rcount = 0, wcount = 0;

    // horas en unidades del fechado gps (1/128 s desde las 00:00)
    long start128 = 0, end128 = 0;

    int skip_midnight = 0;

    struct tm start_date_tm;
    struct tm end_date_tm;
    char start_date_str[256], end_date_str[256];
    time_t start_date_timet;
    time_t end_date_timet;

    if( argc != 8 ) {
	printf("filtertime_s%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("filtertime_s%s in_filename.gps out_filename.gps headerbytes prebytes postbytes start_time end_time\n", ARCH);
	printf("start_time and end_time: HH:MM\n");
	printf("automatic adjustment of midnight crossing\n\n");
	exit(EXIT_SUCCESS);
    }

    header_bytes = atoi(argv[3]);
    pre_bytes = atoi(argv[4]);
    post_bytes = atoi(argv[5]);

    // inicializamos
    memset(start_date_str, 0, 256);
    memset(end_date_str, 0, 256);
    memset(&start_date_tm, 0, sizeof(struct tm));
    memset(&end_date_tm, 0, sizeof(struct tm));

    // formateamos la fecha de entrada en un string
    snprintf(start_date_str, 20, "1970-01-01 %s:00", argv[6]);
    snprintf(end_date_str, 20, "1970-01-01 %s:59", argv[7]);

    // usamos strptime para pasar de string a struct tm
    strptime(start_date_str, "%Y-%m-%d %H:%M:%S", &start_date_tm);
    strptime(end_date_str, "%Y-%m-%d %H:%M:%S", &end_date_tm);

    start_date_timet = mktime(&start_date_tm);
    end_date_timet = mktime(&end_date_tm);

    if ( (time_t) -1 == start_date_timet ) {
	fprintf(stderr, "error in start_time (%s)\n", strerror(errno)); exit(1);
    }

    if ( (time_t) -1 == end_date_timet ) {
	fprintf(stderr, "error in end_time (%s)\n", strerror(errno)); exit(1);
    }

    fprintf(stderr, "start_date(%s=>%ld)/end_date(%s=>%ld)\n",
	start_date_str, start_date_timet, end_date_str, end_date_timet);

    if ( post_bytes != 10 ) {
	fprintf(stderr, "error post bytes must be 10\n"); exit(1);
    }
    if ( header_bytes < 0 || pre_bytes < 0 ) {
	fprintf(stderr, "error headerbytes and prebytes must be >= 0\n"); exit(1);
    }

    // la hora gps (24 bits, < 2^24) cabe exacta en un float, así que
    // comparar raw contra t*128 en enteros equivale a comparar raw/128.0
    // contra t como se hacía antes
    start128 = (long) start_date_timet * 128;
    end128 = (long) end_date_timet * 128;

    if ( !strcmp(argv[1], "-") ) {
	fin = 0;
    } else {
	if ( (fin = open(argv[1], O_RDONLY)) < 0 ) {
	    fprintf(stderr, "error input file (%s)\n", strerror(errno)); exit(1);
	}
    }
    is_regular = (0 == fstat(fin, &st)) && S_ISREG(st.st_mode);
#ifdef POSIX_FADV_SEQUENTIAL
    if ( is_regular )
	posix_fadvise(fin, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

    if ( !strcmp(argv[2], "-") ) {
	fout = 1;
    } else {
	if ( (fout = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0 ) {
	    fprintf(stderr, "error output file (%s)\n", strerror(errno)); exit(1);
	}
    }

    if ( NULL == (buf = malloc(FT_BUFFER_SIZE)) ) {
	fprintf(stderr, "error malloc (%s)\n", strerror(errno)); exit(1);
    }

    if ( 0 != header_bytes ) {
	unsigned char *ptr = malloc(header_bytes);
	size_t count;
	if ( NULL == ptr ) {
	    fprintf(stderr, "error malloc (%s)\n", strerror(errno)); exit(1);
	}
	if ( (size_t) header_bytes != (count = read_full(fin, ptr, header_bytes)) ) {
	    fprintf(stderr, "error reading header header_bytes(%d) count(%zu)\n", header_bytes, count);
	    exit(EXIT_FAILURE);
	}
	write_full(fout, ptr, header_bytes);
	wcount += header_bytes;
	rcount += header_bytes;
	if ( DEBUG ) print_bytes(ptr, header_bytes);
	free(ptr);
    }
    if ( DEBUG ) print_separator();

    for(;;) {
	size_t avail = len - pos, db_bytes, rec_bytes;
	unsigned char *rec, *post;
	long raw;

	// ¿está el registro entero en el buffer? si no, mover la cola al
	// principio y rellenar (antes hay que escribir el tramo pendiente,
	// que apunta a este buffer)
	if ( avail < (size_t) (pre_bytes + 3 + post_bytes) ||
	    avail < (size_t) pre_bytes + post_bytes +
		((buf[pos + pre_bytes + 1] << 8) | buf[pos + pre_bytes + 2]) ) {
	    if ( eof )
		break; // registro final truncado (o fin exacto): se descarta
	    if ( run_len ) {
		write_full(fout, buf + run_start, run_len);
		run_len = 0;
	    }
	    memmove(buf, buf + pos, avail);
	    len = avail;
	    pos = 0;
	    {
		size_t got = read_full(fin, buf + len, FT_BUFFER_SIZE - len);
		if ( got < FT_BUFFER_SIZE - len )
		    eof = 1;
		len += got;
	    }
	    continue;
	}

	rec = buf + pos;
	db_bytes = (rec[pre_bytes + 1] << 8) | rec[pre_bytes + 2];
	if ( db_bytes < 3 ) {
	    if ( run_len )
		write_full(fout, buf + run_start, run_len);
	    fprintf(stderr, "error malformed datablock (size %zu) at offset %lld\n", db_bytes, rcount);
	    exit(EXIT_FAILURE);
	}
	rec_bytes = pre_bytes + db_bytes + post_bytes;
	post = rec + pre_bytes + db_bytes;
	raw = (post[post_bytes - 4] << 16) + (post[post_bytes - 3] << 8) + post[post_bytes - 2];
	pos += rec_bytes;
	rcount += rec_bytes;

	if ( DEBUG ) {
	    print_bytes(rec + pre_bytes, db_bytes);
	    printf("%3.3f\n", raw / 128.0);
	}

	// si la hora del plot está entre las 23:00 y las 23:30, no hacemos el skip
	// de seguridad en la medianoche, se entiene que la grabación viene de antes
	if ( 0 == skip_midnight && raw > 82800L*128 && raw < 84600L*128 ) {
	    skip_midnight = 1;
	}

	// si la grabación empieza más allá de las 23:53, omitimos hasta que sean las 00:00
	if ( 0 == skip_midnight && raw > 86000L*128 ) {
	    if ( DEBUG ) printf("skipping 0\n");
	    continue;
	}
	// una vez pasado este momento, nunca más vamos a omitir
	skip_midnight = 1;

	if ( raw < start128 ) {
	    if ( DEBUG ) printf("skipping 1\n");
	    continue;
	}

	if ( raw > end128 ) {
	    if ( DEBUG ) printf("skipping 2\n");
	    // a partir de aquí no se escribe nada más. Con un fichero
	    // regular se deja de leer. Con stdin se lee y descarta el resto
	    // sin parsear, para que el proceso de delante en el pipe (bzcat,
	    // otro filtro) termine normalmente en vez de morir con SIGPIPE.
	    /* Para cortar también con stdin, sin drenar (pruebas de
	     * integración), descomentar la línea siguiente. Los procesos de
	     * delante en el pipe (bzcat, otro filtro) morirán con SIGPIPE
	     * (código 141, fallo con "set -o pipefail"); los de detrás solo
	     * ven EOF y terminan bien. */
	    /* is_regular = 1; */
	    if ( !is_regular ) {
		size_t got;
		// el drenado reutiliza buf: escribir antes el tramo pendiente
		if ( run_len ) {
		    write_full(fout, buf + run_start, run_len);
		    run_len = 0;
		}
		rcount += len - pos;
		while ( !eof ) {
		    got = read_full(fin, buf, FT_BUFFER_SIZE);
		    if ( got < FT_BUFFER_SIZE )
			eof = 1;
		    rcount += got;
		}
	    }
	    break;
	}

	// registro aceptado: se añade al tramo pendiente si es contiguo
	if ( run_len && run_start + run_len == (size_t) (rec - buf) ) {
	    run_len += rec_bytes;
	} else {
	    if ( run_len )
		write_full(fout, buf + run_start, run_len);
	    run_start = rec - buf;
	    run_len = rec_bytes;
	}
	wcount += rec_bytes;

	if ( DEBUG ) print_separator();
    }
    if ( run_len )
	write_full(fout, buf + run_start, run_len);

    fprintf(stderr, "readed(%lld)/written(%lld)\n", rcount, wcount);

    free(buf);
    if ( 1 != fout && 0 != close(fout) ) {
	fprintf(stderr, "error close fout (%s)\n", strerror(errno)); exit(1);
    }
    if ( 0 != fin && 0 != close(fin) ) {
	fprintf(stderr, "error close fin (%s)\n", strerror(errno)); exit(1);
    }
    exit(EXIT_SUCCESS);
}
