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
    int fdin, fdout;
    int i=0;
    unsigned int lendb, filesize;
    unsigned char *ptr;

    if(argc!=3) {
        printf("hensoldt2ast_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
        printf("hensoldt2ast_%s in_filename.rec out_filename.ast\n\n", ARCH);
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

    i = 0;
    while( i<filesize ) {
	//int j;
	lendb = (ptr[i+0]<<24) + (ptr[i+1]<<16) +
	    (ptr[i+2]<<8) + (ptr[i+3]);
	/*
	printf("lendb(%d)\n", lendb);
	printf("PRE:["); for(j=0;j<12;j++) printf("%02X ", ptr[i+j]); printf("]\n");
	printf("DAT:["); for(j=0;j<lendb-12;j++) printf("%02X ", ptr[i+12+j]); printf("]\n");
        */
        if (write(fdout, ptr + i + 12, lendb - 12) != (lendb-12)) {
	    printf("error write\n"); exit(1);
	}
	i += lendb;
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
