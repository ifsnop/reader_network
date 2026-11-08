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

/* incluir el primero: define _FILE_OFFSET_BITS antes de los includes del
 * sistema */
#include "gps_stream.h"
#include "defines.h"

#define DEBUG 0

/* "HH:MM" (hora de 1 o 2 cifras, minutos de 2) -> segundos desde las
 * 00:00 UTC, que es la referencia del fechado gps. Devuelve -1 si el
 * formato no es exactamente ese o la hora no es válida. No se usa
 * strptime()+mktime(): mktime() interpreta la hora como local y el rango
 * salía desplazado si la máquina no estaba en UTC. */
static long parse_hhmm(const char *s) {
    int h = 0, m = 0, i = 0;
    if ( s[i] < '0' || s[i] > '9' ) return -1;
    h = s[i++] - '0';
    if ( s[i] >= '0' && s[i] <= '9' ) h = h * 10 + (s[i++] - '0');
    if ( s[i++] != ':' ) return -1;
    if ( s[i] < '0' || s[i] > '9' || s[i+1] < '0' || s[i+1] > '9' ) return -1;
    m = (s[i] - '0') * 10 + (s[i+1] - '0');
    if ( s[i+2] != '\0' ) return -1;
    if ( h > 23 || m > 59 ) return -1;
    return h * 3600L + m * 60L;
}

int main(int argc, char *argv[]) {
    int pre_bytes = 0, post_bytes = 0, header_bytes = 0;
    gps_stream gs;
    unsigned char *db;
    size_t db_bytes;

    // horas en unidades del fechado gps (1/128 s desde las 00:00)
    long start128 = 0, end128 = 0;

    int skip_midnight = 0;

    long start_s, end_s;

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

    // segundos desde las 00:00 UTC; end incluye hasta el segundo :59
    if ( (start_s = parse_hhmm(argv[6])) < 0 ) {
	fprintf(stderr, "error in start_time (%s), expected HH:MM\n", argv[6]); exit(1);
    }
    if ( (end_s = parse_hhmm(argv[7])) < 0 ) {
	fprintf(stderr, "error in end_time (%s), expected HH:MM\n", argv[7]); exit(1);
    }
    end_s += 59;

    fprintf(stderr, "start_date(1970-01-01 %02ld:%02ld:%02ld=>%ld)/end_date(1970-01-01 %02ld:%02ld:%02ld=>%ld)\n",
	start_s / 3600, start_s / 60 % 60, start_s % 60, start_s,
	end_s / 3600, end_s / 60 % 60, end_s % 60, end_s);

    if ( post_bytes != 10 ) {
	fprintf(stderr, "error post bytes must be 10\n"); exit(1);
    }

    // en unidades de 1/128 s. end128 es el último instante del segundo :59
    // (HH:MM:59 + 127/128): con end128 = end_s*128 se descartaban los
    // registros de HH:MM:59,008 a HH:MM:59,992 y, al encadenar ventanas
    // consecutivas (00:00-00:59 y 01:00-01:59), se perdía casi 1 s de datos
    // en cada borde. Así las ventanas encajan sin huecos ni solapes.
    start128 = start_s * 128;
    end128 = (end_s + 1) * 128 - 1;

    gs_open(&gs, argv[1], argv[2], header_bytes, pre_bytes, post_bytes);

    while ( NULL != (db = gs_next(&gs, &db_bytes)) ) {
	unsigned char *post = db + db_bytes;
	long raw = (post[post_bytes - 4] << 16) + (post[post_bytes - 3] << 8) + post[post_bytes - 2];

	if ( DEBUG ) {
	    print_bytes(db, db_bytes);
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
	    // a partir de aquí no se escribe nada más: con un fichero se deja
	    // de leer y con stdin se drena sin parsear (ver gs_stop() en
	    // gps_stream.h, que tiene la línea a descomentar para cortar
	    // también con stdin en las pruebas de integración)
	    gs_stop(&gs);
	    break;
	}

	gs_keep(&gs);
	if ( DEBUG ) print_separator();
    }

    gs_close(&gs);
    exit(EXIT_SUCCESS);
}
