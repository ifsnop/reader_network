/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2019 Diego Torres <diego dot torres at gmail dot com>

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
#include "defines.h"

int main(int argc, char *argv[]) {
    int postbytes=10, headerbytes=2200; // formato gps v1 (2200 0 10)
    int fdin, fdout;
    int i=0;
    unsigned long time = 0;
    unsigned int lendb, filesize;
    unsigned char *ptr;
    unsigned char dst_ptr[65536]; // db destino, para guardar header de era antes de escribir en fichero
    
    if(argc!=4) {
        printf("gps2era_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
        printf("filtersacsic_%s in_filename.gps out_filename.era.ast unix_time_to_00:00\n\n", ARCH);
        exit(EXIT_SUCCESS);
    }

    if ( (fdin = open(argv[1], O_RDONLY)) == -1 ) {
	printf("error input file\n"); exit(1);
    }
    if ( (fdout = open(argv[2], O_CREAT|O_WRONLY|O_TRUNC, 0666)) == -1 ) {
	printf("error output file\n"); exit(1);
    }
    if ( (filesize = lseek(fdin, 0, SEEK_END)) == -1 ) {
	printf("error lseek_end file\n"); exit(1);
    }
    if ( (lseek(fdin, 0, SEEK_SET)) != 0 ) {
	printf("error lseek_set file\n"); exit(1);
    }

    if ( (ptr = (unsigned char *) malloc (filesize)) == NULL ) {
	printf("error malloc\n"); exit(1);
    }

    if (read(fdin, ptr, filesize) != filesize) {
	printf("error read\n");	exit(1);
    }

    time = atol(argv[3]);

    i += headerbytes;
    while( i<filesize ) {
        float current_gps_time = 0;
        unsigned long unixtime = 0;

	lendb = (ptr[i+1]<<8) + ptr[i+2];

        current_gps_time = ((ptr[i + lendb + 6]<<16 ) +
            (ptr[i + lendb + 7] << 8) +
            (ptr[i + lendb + 8]) ) / 128.0;
        dst_ptr[0] = 0x38; // ERA TYPE
        dst_ptr[1] = 0; // ERA SUBTYPE, ALWAYS 0
        dst_ptr[2] = (unsigned char) ((lendb + 4) & 0xFF); // length of datablock + 4
        dst_ptr[3] = (unsigned char) ((lendb + 4) >> 8);
        unixtime = (unsigned long) current_gps_time + time;
        dst_ptr[4] = (unsigned char) ((unixtime>>0) & 0xFF);
        dst_ptr[5] = (unsigned char) ((unixtime>>8) & 0xFF);
        dst_ptr[6] = (unsigned char) ((unixtime>>16) & 0xFF);
        dst_ptr[7] = (unsigned char) ((unixtime>>24) & 0xFF);
        memcpy(dst_ptr + 8, ptr + i, lendb);
	//{
	// int j;
	// printf("%ld %3.3f\n", unixtime, current_gps_time);
	// printf("cat(%02X) len(%d) pre(%d) post(%d)\n", ptr[i], lendb - prebytes - postbytes, prebytes,postbytes);
	// for(j=0; j < lendb + postbytes; j++) printf("%02X ", ptr[i+j]); printf("\n");
	// for(j=0; j < 8 + lendb; j++) printf("%02X ", dst_ptr[j]); printf("\n====\n");
        //}
        if (write(fdout, dst_ptr, lendb + 8) != (lendb+8)) {
            printf("error write\n"); exit(1);
        }
	i += lendb + postbytes;
    }

    free(ptr);
    if (close(fdout) == -1) {
	printf("error close fdout\n"); exit(1);
    }
    if (close(fdin) == -1) {
	printf("error close fdin\n"); exit(1);
    }
    exit(EXIT_SUCCESS);
}
