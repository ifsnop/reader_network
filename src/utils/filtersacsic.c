/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2014 Diego Torres <diego dot torres at gmail dot com>

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
    int prebytes=0, postbytes=0, headerbytes=0;
    int fdin, fdout, isac, isic;
    int i=0;
    unsigned char sac = '\0', sic = '\0';
    unsigned int lendb, filesize;
    unsigned char *ptr;

    if(argc!=8) {
        printf("filtersacsic_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
        printf("filtersacsic_%s in_filename.gps out_filename.gps headerbytes prebytes postbytes sac sic\n\n", ARCH);
        exit(EXIT_SUCCESS);
    }

    headerbytes = atoi(argv[3]);
    prebytes = atoi(argv[4]);
    postbytes = atoi(argv[5]);
    isac = atoi(argv[6]); isic = atoi(argv[7]);
    sac = (isac % 255); sic = (isic % 255);

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

    if (write(fdout, ptr, headerbytes) != headerbytes) {
        printf("error write\n"); exit(1);
    }

    i+=headerbytes;
    while( i<filesize ) {
        int oldi = i;
        int match = 0;
        int fspeclength = 0;

	//int j;

	i += prebytes;
	lendb = (ptr[i+1]<<8) + ptr[i+2] + postbytes;
        // comprobar si hay que escribir por sac/sic

        if ((ptr[i+3] & 128) == 128) {
            while((ptr[i+3+fspeclength] & 1) == 1) fspeclength++;
            //printf("sac(%02X) sic(%02X) sac(%02X) sic(%02X) isac(%d) isic(%d)\n", ptr[i+3+fspeclength+1], ptr[i+3+fspeclength+2], sac, sic, isac, isic);
            if ((ptr[i+3+fspeclength+1] == sac) &&
                (ptr[i+3+fspeclength+2] == sic) ) {
                match=1;
            }
        }

	//printf("cat(%02X) len(%d) pre(%d) post(%d)\n", ptr[i], lendb - prebytes - postbytes, prebytes,postbytes);
	//for(j=0;j<lendb+prebytes+postbytes;j++) printf("%02X ", ptr[i+j-prebytes]); printf("==\n\n");

        if (match) {
            if (write(fdout, ptr + oldi, lendb) != lendb) {
	        printf("error write\n"); exit(1);
	    }
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
