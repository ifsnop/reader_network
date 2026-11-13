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

#define JG_HEADER_BYTES 2200
#define JG_POST_BYTES 10

/* la hora gps es la hora del día en 1/128 s: vuelve a 0 a medianoche y el
 * fichero no lleva la fecha */
/* jg_advance() se llama por cada registro desde varios sitios y gcc no la
 * inlinea por su cuenta: forzarlo ahorra ~15 instrucciones por registro
 * (always_inline existe desde gcc 3.1; en otros compiladores, no-op) */
#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define JG_HOT __inline__ __attribute__((always_inline))
#else
#define JG_HOT __inline__
#endif

#define JG_DAY128 (86400LL * 128)
#define JG_HALF_DAY128 (43200LL * 128)

typedef struct {
    gps_stream gs;
    const char *name;
    long long key;            /* hora del registro actual con el día sumado */
    long long ord;            /* key * nin + (nin - 1 - índice): orden de salida */
    long long rank;           /* nin - 1 - índice */
    long long nin;
    long long day_off;        /* días (en 1/128 s) sumados a la hora gps */
    long long start_day_off;  /* el de su primer registro */
    long prev_ts;             /* hora gps del registro anterior, -1 al empezar */
    long backsteps;           /* veces que la hora va hacia atrás (< 12 h) */
    long crossings;           /* medianoches cruzadas dentro del fichero */
} jg_input;

/* hora gps del registro en 1/128 s (bytes 6-8 del trailer) */
static long ts128(const unsigned char *db, size_t db_bytes) {
    const unsigned char *post = db + db_bytes;
    return (post[JG_POST_BYTES - 4] << 16) + (post[JG_POST_BYTES - 3] << 8) + post[JG_POST_BYTES - 2];
}

/* Lee el siguiente registro de la entrada y calcula su clave. Dentro de un
 * fichero la hora solo puede retroceder más de 12 h al cruzar la medianoche:
 * entonces se suma un día. Un retroceso menor (reloj ajustado) se cuenta
 * para avisar. Devuelve 0 al acabarse la entrada. */
static JG_HOT int jg_advance(jg_input *in) {
    size_t n;
    unsigned char *db = gs_next(&in->gs, &n);
    long ts;

    if ( NULL == db )
	return 0;
    ts = ts128(db, n);
    if ( ts < in->prev_ts ) {
	if ( in->prev_ts - ts > JG_HALF_DAY128 ) {
	    in->day_off += JG_DAY128;
	    in->crossings++;
	} else {
	    in->backsteps++;
	}
    }
    in->prev_ts = ts;
    in->key = ts + in->day_off;
    in->ord = in->key * in->nin + in->rank;
    return 1;
}

/* orden de salida: por clave y, con la misma, la entrada posterior primero
 * (con dos ficheros, el 2 antes que el 1, como siempre). Va todo en ord para
 * que sea una sola comparación: el desempate en dos pasos fallaba más en la
 * predicción de saltos y costaba un 10 % con dos entradas muy intercaladas */
static __inline__ int jg_before(const jg_input *in, int a, int b) {
    return in[a].ord < in[b].ord;
}

static void jg_sift_down(const jg_input *in, int *heap, int n, int i) {
    for (;;) {
	int c = 2 * i + 1, t;
	if ( c >= n )
	    break;
	if ( c + 1 < n && jg_before(in, heap[c + 1], heap[c]) )
	    c++;
	if ( !jg_before(in, heap[c], heap[i]) )
	    break;
	t = heap[i]; heap[i] = heap[c]; heap[c] = t;
	i = c;
    }
}

/* Mezcla de las dos últimas entradas vivas (a va antes que b). Es el caso
 * más habitual y, con ficheros muy intercalados, se cambia de entrada casi
 * en cada registro: aquí cambiar es solo intercambiar los punteros, sin el
 * heap (con el bucle general costaba un 10 % más con dos entradas). */
static void jg_merge2(jg_input *a, jg_input *b) {
    for (;;) {
	jg_input *t;
	do {
	    gs_keep(&a->gs);
	    if ( !jg_advance(a) ) {
		// se ha acabado a: el resto de b va entero
		do {
		    gs_keep(&b->gs);
		} while ( jg_advance(b) );
		return;
	    }
	} while ( a->ord < b->ord );
	t = a; a = b; b = t;
    }
}

static const char *jg_label(const char *name) {
    return strcmp(name, "-") ? name : "stdin";
}

/* La salida no puede ser una de las entradas: open(O_TRUNC) la vaciaría
 * antes de leerla (y con stdout en modo append no terminaría nunca). */
static void jg_check_output(const char *out, char **ins, int nin) {
    struct stat so, si;
    int i;

    if ( !strcmp(out, "-") ) {
	if ( 0 != fstat(1, &so) || !S_ISREG(so.st_mode) )
	    return;
    } else if ( 0 != stat(out, &so) ) {
	return;
    }
    for ( i = 0; i < nin; i++ ) {
	if ( !strcmp(ins[i], "-") ) {
	    if ( 0 != fstat(0, &si) )
		continue;
	} else if ( 0 != stat(ins[i], &si) ) {
	    continue; // gs_reader_open() dará el error
	}
	if ( si.st_dev == so.st_dev && si.st_ino == so.st_ino ) {
	    fprintf(stderr, "error output %s is the same file as input %d (%s)\n",
		strcmp(out, "-") ? out : "stdout", i + 1, jg_label(ins[i]));
	    exit(EXIT_FAILURE);
	}
    }
}

/* Mezcla N ficheros .gps por la hora gps de cada registro, suponiendo que
 * cada entrada viene en orden. La cabecera de la salida es la del fichero 1;
 * las demás se leen y se descartan.
 *
 * Días: el primer registro de cada entrada se coloca a menos de 12 h del
 * primero de la entrada anterior en la línea de comandos (23:59 y 00:00 son
 * días consecutivos, en cualquier orden), y dentro de cada fichero un
 * retroceso de más de 12 h es un cruce de medianoche. Para juntar varios
 * días, pasar los ficheros en orden cronológico.
 *
 * Mezcla: min-heap de entradas; se escribe de la primera mientras vaya antes
 * que la siguiente del heap (un tramo cuesta una comparación por registro y
 * gs_keep() junta los registros seguidos de un mismo fichero). Con dos
 * entradas vivas, jg_merge2().
 *
 * Al final se avisa de lo que puede dejar la salida mal mezclada o
 * incompleta: un registro final cortado (se descarta) y una entrada cuya
 * hora retrocede sin cruzar la medianoche (la salida no queda ordenada). */
int main(int argc, char *argv[]) {
    gs_writer w;
    jg_input *in;
    int *heap;
    int nin, hn = 0, i, nstdin = 0, ref_set = 0;
    long long ref = 0, rtotal = 0;

    if ( argc < 4 ) {
	printf("joingps_s%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("joingps_s%s in_filename_1.gps in_filename_2.gps [... in_filename_n.gps] out_filename.gps\n", ARCH);
	printf("merges the records of all the inputs by gps time; the header is the one of in_filename_1\n");
	printf("one input can be stdin (-), out_filename.gps can be stdout (-)\n");
	printf("midnight: each input is taken to start less than 12 h away from the previous one,\n");
	printf("to join several days give the inputs in chronological order\n");
	printf("on equal times the later input goes first; 4 MiB of memory per input\n\n");
	exit(EXIT_SUCCESS);
    }
    nin = argc - 2;

    for ( i = 0; i < nin; i++ )
	if ( !strcmp(argv[1 + i], "-") )
	    nstdin++;
    if ( nstdin > 1 ) {
	fprintf(stderr, "error only one input can be stdin (-)\n"); exit(EXIT_FAILURE);
    }
    jg_check_output(argv[argc - 1], argv + 1, nin);

    if ( NULL == (in = calloc(nin, sizeof(*in))) || NULL == (heap = malloc(nin * sizeof(*heap))) ) {
	fprintf(stderr, "error malloc (%s)\n", strerror(errno)); exit(EXIT_FAILURE);
    }

    gs_writer_open(&w, argv[argc - 1]);
    for ( i = 0; i < nin; i++ ) {
	in[i].name = argv[1 + i];
	in[i].prev_ts = -1;
	in[i].nin = nin;
	in[i].rank = nin - 1 - i;
	gs_reader_open(&in[i].gs, in[i].name, JG_HEADER_BYTES, 0, JG_POST_BYTES, &w, 0 == i);
    }

    // primer registro de cada entrada y su día respecto a la anterior
    for ( i = 0; i < nin; i++ ) {
	if ( !jg_advance(&in[i]) )
	    continue; // vacía
	if ( ref_set ) {
	    while ( in[i].key < ref - JG_HALF_DAY128 ) {
		in[i].key += JG_DAY128;
		in[i].day_off += JG_DAY128;
	    }
	    while ( in[i].key >= ref + JG_HALF_DAY128 ) {
		in[i].key -= JG_DAY128;
		in[i].day_off -= JG_DAY128;
	    }
	    in[i].ord = in[i].key * nin + in[i].rank;
	}
	in[i].start_day_off = in[i].day_off;
	ref = in[i].key;
	ref_set = 1;
	heap[hn++] = i;
    }
    for ( i = hn / 2 - 1; i >= 0; i-- )
	jg_sift_down(in, heap, hn, i);

    // con tres o más, heap; con dos, jg_merge2()
    while ( hn > 2 ) {
	int m = heap[0];
	int c = jg_before(in, heap[2], heap[1]) ? heap[2] : heap[1];
	jg_input *im = &in[m];
	long long cord = in[c].ord;
	// m va antes que c (es la cima): se escribe hasta que deje de ir antes
	do {
	    gs_keep(&im->gs);
	    if ( !jg_advance(im) ) {
		heap[0] = heap[--hn];
		break;
	    }
	} while ( im->ord < cord );
	jg_sift_down(in, heap, hn, 0);
    }
    if ( 2 == hn ) {
	jg_merge2(&in[heap[0]], &in[heap[1]]);
    } else if ( 1 == hn ) {
	jg_input *im = &in[heap[0]];
	do {
	    gs_keep(&im->gs);
	} while ( jg_advance(im) );
    }

    for ( i = 0; i < nin; i++ ) {
	jg_input *p = &in[i];
	size_t tail = gs_tail_bytes(&p->gs);
	gs_flush(&p->gs);
	rtotal += p->gs.rcount;
	if ( 0 != p->start_day_off )
	    fprintf(stderr, "input %d (%s): starts %+lld day(s) from input 1\n",
		i + 1, jg_label(p->name), p->start_day_off / JG_DAY128);
	if ( p->crossings )
	    fprintf(stderr, "input %d (%s): crosses midnight %ld time(s)\n",
		i + 1, jg_label(p->name), p->crossings);
	if ( tail )
	    fprintf(stderr, "warning: input %d (%s): incomplete last record, %zu bytes discarded\n",
		i + 1, jg_label(p->name), tail);
	if ( p->backsteps )
	    fprintf(stderr, "warning: input %d (%s): time goes backwards %ld time(s), the output is not fully sorted\n",
		i + 1, jg_label(p->name), p->backsteps);
    }
    gs_flush_out(&w);
    fprintf(stderr, "readed(%lld)/written(%lld)\n", rtotal, w.wcount);
    // todo lo leído (sin las cabeceras de las entradas 2..n) tiene que estar en la salida
    if ( rtotal - (long long) (nin - 1) * JG_HEADER_BYTES != w.wcount ) {
	fprintf(stderr, "error internal: %lld bytes read but not written\n",
	    rtotal - (long long) (nin - 1) * JG_HEADER_BYTES - w.wcount);
	exit(EXIT_FAILURE);
    }
    gs_writer_close(&w);
    for ( i = 0; i < nin; i++ )
	gs_reader_close(&in[i].gs);
    free(heap);
    free(in);
    exit(EXIT_SUCCESS);
}
