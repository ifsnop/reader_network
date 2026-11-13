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

/* Lectura y escritura de ficheros .gps para las utilidades *_s de src/utils
 * (filtertime_s, filtercat_s, filtersacsic_s, joingps_s, cleanast_s,
 * listersacsic_s).
 *
 * Formato: header_bytes de cabecera y, por registro, pre_bytes + datablock
 * ASTERIX (cat, longitud de 16 bits incluyendo esos 3 bytes, datos) +
 * post_bytes (el trailer gps de reader_network, 10 bytes).
 *
 * Lectura (gps_stream): read() en bloques de GS_BUFFER_SIZE y los registros
 * se parsean dentro del propio buffer. Antes, cada utilidad hacía 3 fread
 * por registro.
 *
 * Escritura (gs_writer): los registros aceptados consecutivos de un mismo
 * lector forman un tramo. Si es grande (>= GS_DIRECT_MIN, p.ej. filtertime_s,
 * cuya salida es casi siempre un único tramo) se escribe directamente desde
 * el buffer de entrada; si es pequeño (filtros que descartan registros
 * sueltos intercalados, joingps_s alternando entre varios ficheros) se copia a
 * un buffer de salida de GS_OUT_SIZE que se escribe al llenarse. Escribir
 * cada tramo pequeño con su propio write() costaba millones de llamadas al
 * sistema y era más lento que el stdio anterior. gs_emit() añade bytes
 * cualesquiera (solo el datablock en cleanast_s, texto en listersacsic_s).
 * Varios lectores pueden compartir un gs_writer: el orden de la salida es
 * el de las llamadas a gs_keep()/gs_emit().
 *
 * Uso con una entrada y una salida:
 *     gps_stream gs;
 *     gs_open(&gs, argv[1], argv[2], header_bytes, pre_bytes, post_bytes);
 *     while ( NULL != (db = gs_next(&gs, &db_bytes)) )
 *         if ( interesa(db, db_bytes) ) gs_keep(&gs);
 *     gs_close(&gs);
 *
 * Con varias entradas o sin copiar la cabecera: gs_writer_open(),
 * gs_reader_open() por cada entrada, gs_reader_close() y gs_writer_close().
 *
 * Este fichero se incluye desde cada utilidad (cada una se compila como un
 * único .c en build.sh); las funciones son static __inline__ para que las
 * que no use una utilidad no provoquen avisos de -Wunused-function.
 */

#ifndef GPS_STREAM_H
#define GPS_STREAM_H

/* ficheros > 2 GiB también en el binario de 32 bits (no-op en 64 bits y
 * en libc antiguas que no lo conocen). Tiene que ir antes de cualquier
 * include del sistema: incluir este fichero el primero. */
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* un registro ocupa como máximo pre_bytes + 65535 + post_bytes */
#define GS_BUFFER_SIZE (4*1024*1024)
#define GS_OUT_SIZE (1024*1024)
#define GS_DIRECT_MIN (256*1024)

struct gps_stream;

typedef struct {
    int fd;                   /* -1: sin salida (se descarta lo que se escriba) */
    unsigned char *out;       /* buffer para los tramos pequeños y gs_emit() */
    size_t out_len;
    struct gps_stream *run_owner; /* lector con un tramo pendiente */
    long long wcount;
} gs_writer;

typedef struct gps_stream {
    int fin;
    int is_regular;           /* la entrada es un fichero regular (no un pipe) */
    int eof;
    int pre_bytes, post_bytes;
    unsigned char *buf;
    size_t len, pos;          /* bytes válidos en buf / siguiente registro */
    size_t run_start, run_len; /* tramo de registros aceptados pendiente de escribir */
    size_t rec, rec_bytes;    /* registro actual (offset en buf, con pre y post) */
    long long rcount;
    const char *name;         /* para los mensajes de error */
    gs_writer *w;
    gs_writer own_w;          /* el de gs_open() */
} gps_stream;

static __inline__ void print_bytes(unsigned char* p, ssize_t count) {
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

static __inline__ void print_separator(void) {
    fprintf(stderr, "==============================================\n");
    return;
}

/* lee hasta count bytes (menos solo en EOF) */
static __inline__ size_t gs_read_full(int fd, unsigned char *p, size_t count) {
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

static __inline__ void gs_write_full(int fd, const unsigned char *p, size_t count) {
    if ( fd < 0 )
	return;
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

/* ---- salida ---- */

/* "-" = stdout, NULL = sin salida */
static __inline__ void gs_writer_open(gs_writer *w, const char *out) {
    memset(w, 0, sizeof(*w));
    if ( NULL == out ) {
	w->fd = -1;
    } else if ( !strcmp(out, "-") ) {
	w->fd = 1;
    } else {
	if ( (w->fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0 ) {
	    fprintf(stderr, "error output file (%s)\n", strerror(errno)); exit(1);
	}
    }
    if ( NULL == (w->out = malloc(GS_OUT_SIZE)) ) {
	fprintf(stderr, "error malloc (%s)\n", strerror(errno)); exit(1);
    }
}

static __inline__ void gs_flush_out(gs_writer *w) {
    if ( w->out_len ) {
	gs_write_full(w->fd, w->out, w->out_len);
	w->out_len = 0;
    }
}

/* copia al buffer de salida (o escribe directamente si es grande) */
static __inline__ void gs_put(gs_writer *w, const unsigned char *p, size_t n) {
    if ( n >= GS_DIRECT_MIN ) {
	gs_flush_out(w);
	gs_write_full(w->fd, p, n);
	return;
    }
    if ( w->out_len + n > GS_OUT_SIZE )
	gs_flush_out(w);
    memcpy(w->out + w->out_len, p, n);
    w->out_len += n;
}

/* Saca el tramo pendiente del lector (hay que hacerlo antes de reutilizar
 * su buf y antes de escribir cualquier otra cosa en su writer). */
static __inline__ void gs_flush(gps_stream *gs) {
    if ( gs->run_len ) {
	gs_put(gs->w, gs->buf + gs->run_start, gs->run_len);
	gs->run_len = 0;
    }
    if ( gs->w->run_owner == gs )
	gs->w->run_owner = NULL;
}

/* lo que esté pendiente en otro lector va antes */
static __inline__ void gs_flush_owner(gs_writer *w) {
    if ( NULL != w->run_owner )
	gs_flush(w->run_owner);
}

/* añade n bytes cualesquiera a la salida, en orden con los registros */
static __inline__ void gs_emit(gs_writer *w, const unsigned char *p, size_t n) {
    gs_flush_owner(w);
    gs_put(w, p, n);
    w->wcount += n;
}

static __inline__ void gs_writer_close(gs_writer *w) {
    gs_flush_owner(w);
    gs_flush_out(w);
    free(w->out);
    w->out = NULL;
    if ( w->fd > 1 && 0 != close(w->fd) ) {
	fprintf(stderr, "error close fout (%s)\n", strerror(errno)); exit(1);
    }
}

/* ---- entrada ---- */

/* Abre la entrada ("-" = stdin) y lee la cabecera; si copy_header, la
 * escribe en w. */
static __inline__ void gs_reader_open(gps_stream *gs, const char *in,
    int header_bytes, int pre_bytes, int post_bytes, gs_writer *w, int copy_header) {
    struct stat st;

    memset(gs, 0, sizeof(*gs));
    if ( header_bytes < 0 || pre_bytes < 0 || post_bytes < 0 ) {
	fprintf(stderr, "error headerbytes, prebytes and postbytes must be >= 0\n"); exit(1);
    }
    gs->pre_bytes = pre_bytes;
    gs->post_bytes = post_bytes;
    gs->w = w;
    gs->name = strcmp(in, "-") ? in : "stdin";

    if ( !strcmp(in, "-") ) {
	gs->fin = 0;
    } else {
	if ( (gs->fin = open(in, O_RDONLY)) < 0 ) {
	    fprintf(stderr, "error input file %s (%s)\n", in, strerror(errno)); exit(1);
	}
    }
    gs->is_regular = (0 == fstat(gs->fin, &st)) && S_ISREG(st.st_mode);
#ifdef POSIX_FADV_SEQUENTIAL
    if ( gs->is_regular )
	posix_fadvise(gs->fin, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

    if ( NULL == (gs->buf = malloc(GS_BUFFER_SIZE)) ) {
	fprintf(stderr, "error malloc (%s)\n", strerror(errno)); exit(1);
    }

    if ( 0 != header_bytes ) {
	size_t count;
	if ( (size_t) header_bytes > GS_BUFFER_SIZE ) {
	    fprintf(stderr, "error headerbytes too big (%d)\n", header_bytes); exit(1);
	}
	if ( (size_t) header_bytes != (count = gs_read_full(gs->fin, gs->buf, header_bytes)) ) {
	    fprintf(stderr, "error reading header of %s header_bytes(%d) count(%zu)\n",
		gs->name, header_bytes, count);
	    exit(EXIT_FAILURE);
	}
	gs->rcount += header_bytes;
	if ( copy_header )
	    gs_emit(w, gs->buf, header_bytes);
    }
}

/* una entrada y una salida ("-" = stdin/stdout), copiando la cabecera */
static __inline__ void gs_open(gps_stream *gs, const char *in, const char *out,
    int header_bytes, int pre_bytes, int post_bytes) {
    gs_writer w;
    // el writer vive dentro de gs: se abre en una variable local y se
    // copia después, porque gs_reader_open() pone gs a cero
    gs_writer_open(&w, out);
    gs_reader_open(gs, in, header_bytes, pre_bytes, post_bytes, &w, 0);
    gs->own_w = w;
    gs->w = &gs->own_w;
    if ( 0 != header_bytes )
	gs_emit(gs->w, gs->buf, header_bytes);
}

/* Siguiente registro: devuelve el datablock (tras los pre_bytes) y su
 * tamaño en *db_bytes; el trailer empieza en db + *db_bytes. NULL al
 * terminar (un registro final incompleto se descarta). Un datablock con
 * tamaño < 3 es un error: se escribe lo ya aceptado y se termina. */
static __inline__ unsigned char *gs_next(gps_stream *gs, size_t *db_bytes) {
    size_t fixed = gs->pre_bytes + 3 + gs->post_bytes;
    for (;;) {
	size_t avail = gs->len - gs->pos;
	unsigned char *rec = gs->buf + gs->pos;
	size_t size;

	if ( avail >= fixed ) {
	    size = (rec[gs->pre_bytes + 1] << 8) | rec[gs->pre_bytes + 2];
	    if ( avail >= fixed - 3 + size ) {
		if ( size < 3 ) {
		    gs_flush_owner(gs->w);
		    gs_flush_out(gs->w);
		    fprintf(stderr, "error malformed datablock (size %zu) at offset %lld of %s\n", size, gs->rcount, gs->name);
		    exit(EXIT_FAILURE);
		}
		gs->rec = gs->pos;
		gs->rec_bytes = fixed - 3 + size;
		gs->pos += gs->rec_bytes;
		gs->rcount += gs->rec_bytes;
		*db_bytes = size;
		return rec + gs->pre_bytes;
	    }
	}

	// el registro no está entero en el buffer: mover la cola al principio
	// y rellenar (antes hay que escribir el tramo pendiente, que apunta a
	// este buffer)
	if ( gs->eof )
	    return NULL;
	gs_flush(gs);
	memmove(gs->buf, gs->buf + gs->pos, avail);
	gs->len = avail;
	gs->pos = 0;
	{
	    size_t got = gs_read_full(gs->fin, gs->buf + gs->len, GS_BUFFER_SIZE - gs->len);
	    if ( got < GS_BUFFER_SIZE - gs->len )
		gs->eof = 1;
	    gs->len += got;
	}
    }
}

/* bytes que sobran al final de la entrada (un registro incompleto que
 * gs_next() ha descartado); solo tiene sentido después de que gs_next()
 * devuelva NULL */
static __inline__ size_t gs_tail_bytes(const gps_stream *gs) {
    return gs->len - gs->pos;
}

/* el registro devuelto por el último gs_next() va a la salida */
static __inline__ void gs_keep(gps_stream *gs) {
    gs_writer *w = gs->w;
    if ( w->run_owner != gs ) {
	gs_flush_owner(w);
	w->run_owner = gs;
    }
    if ( gs->run_len && gs->run_start + gs->run_len == gs->rec ) {
	gs->run_len += gs->rec_bytes;
    } else {
	if ( gs->run_len )
	    gs_put(w, gs->buf + gs->run_start, gs->run_len);
	gs->run_start = gs->rec;
	gs->run_len = gs->rec_bytes;
    }
    w->wcount += gs->rec_bytes;
}

/* No se va a escribir nada más de esta entrada. Con un fichero regular se
 * deja de leer; con stdin se lee y descarta el resto sin parsear, para que
 * el proceso de delante en el pipe (bzcat, otro filtro) termine
 * normalmente en vez de morir con SIGPIPE. Después, gs_next() devuelve
 * NULL. */
static __inline__ void gs_stop(gps_stream *gs) {
    /* Para cortar también con stdin, sin drenar (pruebas de integración),
     * descomentar la línea siguiente. Los procesos de delante en el pipe
     * (bzcat, otro filtro) morirán con SIGPIPE (código 141, fallo con
     * "set -o pipefail"); los de detrás solo ven EOF y terminan bien. */
    /* gs->is_regular = 1; */
    gs_flush(gs); // el drenado reutiliza buf
    if ( !gs->is_regular ) {
	gs->rcount += gs->len - gs->pos;
	while ( !gs->eof ) {
	    size_t got = gs_read_full(gs->fin, gs->buf, GS_BUFFER_SIZE);
	    if ( got < GS_BUFFER_SIZE )
		gs->eof = 1;
	    gs->rcount += got;
	}
    }
    gs->eof = 1;
    gs->len = gs->pos = 0;
}

static __inline__ void gs_reader_close(gps_stream *gs) {
    gs_flush(gs);
    free(gs->buf);
    gs->buf = NULL;
    if ( 0 != gs->fin && 0 != close(gs->fin) ) {
	fprintf(stderr, "error close fin (%s)\n", strerror(errno)); exit(1);
    }
}

/* cierra lo abierto con gs_open() e informa de lo leído/escrito */
static __inline__ void gs_close(gps_stream *gs) {
    gs_flush(gs);
    gs_flush_out(gs->w);
    fprintf(stderr, "readed(%lld)/written(%lld)\n", gs->rcount, gs->w->wcount);
    gs_reader_close(gs);
    gs_writer_close(gs->w);
}

/* ---- utilidades comunes ---- */

/* Argumento de filtro con signo leído del texto ("-0" es negativo, cosa que
 * atoi() no distingue). Devuelve el valor absoluto (0..max) y pone *neg; -1
 * si no es un entero en ese rango. */
static __inline__ int gs_parse_signed(const char *s, int max, int *neg) {
    long v = 0;
    *neg = 0;
    if ( '-' == *s ) {
	*neg = 1;
	s++;
    } else if ( '+' == *s ) {
	s++;
    }
    if ( *s < '0' || *s > '9' )
	return -1;
    while ( *s >= '0' && *s <= '9' ) {
	v = v * 10 + (*s++ - '0');
	if ( v > max )
	    return -1;
    }
    if ( '\0' != *s )
	return -1;
    return (int) v;
}

/* Devuelve 1 y el SAC/SIC del primer registro del datablock si este trae
 * I0xx/010 (bit 0x80 del primer byte del FSPEC; es el FRN 1 en las
 * categorías habituales). El FSPEC y el SAC/SIC tienen que caber en el
 * datablock. */
static __inline__ int gs_get_sacsic(const unsigned char *db, size_t db_bytes,
    unsigned char *sac, unsigned char *sic) {
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

#endif
