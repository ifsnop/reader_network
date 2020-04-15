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
#include "defines.h"


float getTS(unsigned char *ptr, int pre, int post, int lendb, unsigned int i) {
    float ts = ((ptr[i + pre + lendb + 6]<<16 ) +
	(ptr[i + pre + lendb + 7] << 8) +
	(ptr[i + pre + lendb + 8]) ) / 128.0;
    return ts;
}

void printDB(unsigned char *ptr, int pre, int post, int lendb, unsigned int i) {
    int j;

    printf("cat(%02X) len(%d) pre(%d) post(%d)\n", ptr[i], lendb,  pre, post);
    for(j=0;j<pre;j++) 
	printf("%02X ", ptr[i+j]);
    printf("|");
    for(j=0;j<lendb;j++)
	printf("%02X ", ptr[i+pre+j]);
    printf("|");
    for(j=0;j<post;j++)
	printf("%02X ", ptr[i+pre+lendb+j]);
    printf("==\n\n");
    return;
}

int main(int argc, char *argv[]) {
    int pre=0, post=0, header=0;
    int fdin1, fdin2, fdout;
    unsigned int i=0, j=0;
    unsigned int lendb1, lendb2, filesize1, filesize2;
    unsigned char *ptr1, *ptr2;
    float ts1 = -1, ts2 = -1, oldts1 = -1, oldts2 = -1, offsetts1 = 0, offsetts2 = 0;

    if(argc!=7) {
	printf("joingps_%s" COPYRIGHT_NOTICE, ARCH, VERSION);
	printf("joingps_%s in_filename_1.gps in_filename_2.gps headerbytes prebytes postbytes out_filename.gps\n\n", ARCH);
	exit(EXIT_SUCCESS);
    }

    header = atoi(argv[3]);
    pre = atoi(argv[4]);
    post = atoi(argv[5]);

    if ( (fdin1 = open(argv[1], O_RDONLY)) == -1 ) {
	printf("error input file 1\n"); exit(1);
    }
    if ( (fdin2 = open(argv[2], O_RDONLY)) == -1 ) {
	printf("error input file 2\n"); exit(1);
    }
    if ( (fdout = open(argv[6], O_CREAT|O_WRONLY|O_TRUNC, 0666)) == -1 ) {
	printf("error output file\n"); exit(1);
    }
    if ( (filesize1 = lseek(fdin1, 0, SEEK_END)) == -1 ) {
	printf("error lseek_end file 1\n"); exit(1);
    }
    if ( (lseek(fdin1, 0, SEEK_SET)) != 0 ) {
	printf("error lseek_set file 1\n"); exit(1);
    }
    if ( (filesize2 = lseek(fdin2, 0, SEEK_END)) == -1 ) {
	printf("error lseek_end file 2\n"); exit(1);
    }
    if ( (lseek(fdin2, 0, SEEK_SET)) != 0 ) {
	printf("error lseek_set file 2\n"); exit(1);
    }

    if (filesize1 < (header + pre + post + 3)) {
	printf("error input file 1 size error\n"); exit(1);
    }
    if (filesize2 < (header + pre + post + 3)) {
	printf("error input file 2 size error\n"); exit(1);
    }

    if ( (ptr1 = (unsigned char *) malloc (filesize1)) == NULL ) {
	printf("error malloc 1\n"); exit(1);
    }
    if ( (ptr2 = (unsigned char *) malloc (filesize2)) == NULL ) {
	printf("error malloc 2\n"); exit(1);
    }

    if (read(fdin1, ptr1, filesize1) != filesize1) {
	printf("error read 1\n");	exit(1);
    }
    if (read(fdin2, ptr2, filesize2) != filesize2) {
	printf("error read 2\n");	exit(1);
    }

    if (write(fdout, ptr1, header) != header) {
	printf("error write\n"); exit(1);
    }

    i += header;
    j += header;

    lendb1 = -1; lendb2 = -1;
    while (i<filesize1 && j<filesize2) {
	if (lendb1==-1) {
	    lendb1 = (ptr1[i+pre+1]<<8) + ptr1[i+pre+2];
	    oldts1 = ts1;
	    ts1 = getTS(ptr1, pre, post, lendb1, i) + offsetts1;
	    if (abs(ts1-oldts1) > 80000) {
		//printf("ADDING offsetts1\n");
		offsetts1 += 86400;
		ts1 += offsetts1;
	    }
	}

	if (lendb2==-1) {
	    lendb2 = (ptr2[j+pre+1]<<8) + ptr2[j+pre+2];
	    oldts2 = ts2;
	    ts2 = getTS(ptr2, pre, post, lendb2, j) + offsetts2;
	    if (abs(ts2-oldts2) > 80000) {
		//printf("ADDING offsetts2\n");
		offsetts2 += 86400;
		ts2 += offsetts2;
	    }
	}

	//printDB(ptr1, pre, post, lendb1, i);
	//printDB(ptr2, pre, post, lendb2, j);
	//printf("%3.2f %3.2f\n", ts1, ts2);

	if (ts1<ts2) {
	    if (write(fdout, ptr1 + i, lendb1+pre+post) != lendb1+pre+post) {
		printf("error write from file 1\n"); exit(1);
	    }
	    i += pre + post + lendb1;
	    lendb1=-1;
	    //printf("·");
	} else {
	    if (write(fdout, ptr2 + j, lendb2+pre+post) != lendb2+pre+post) {
		printf("error write from file 2\n"); exit(1);
	    }
	    j += pre + post + lendb2;
	    lendb2=-1;
	    //printf("+");
	}
    }

    while (i<filesize1) {
	lendb1 = (ptr1[i+pre+1]<<8) + ptr1[i+pre+2];
	if (write(fdout, ptr1 + i, lendb1+pre+post) != lendb1+pre+post) {
	    printf("error write 1\n"); exit(1);
	}
	i += pre + post + lendb1;
    }

    while (j<filesize2) {
	lendb2 = (ptr2[j+pre+1]<<8) + ptr2[j+pre+2];
	if (write(fdout, ptr2 + j, lendb2+pre+post) != lendb2+pre+post) {
	    printf("error write 2\n"); exit(1);
	}
	j += pre + post + lendb2;
    }

    /*

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
    */

    free(ptr1);
    free(ptr2);
    if (close(fdout) == -1) {
	printf("error close fdout\n"); exit(1);
    }
    if (close(fdin2) == -1) {
	printf("error close file 2\n"); exit(1);
    }
    if (close(fdin1) == -1) {
	printf("error close file 1\n"); exit(1);
    }

    exit(EXIT_SUCCESS);
}
