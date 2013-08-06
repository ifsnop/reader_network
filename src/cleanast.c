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
    int fdin, fdout;
    int i=0;
    unsigned int lendb, filesize;
    unsigned char *ptr;

    if(argc!=6) {
	printf("clenast %s in_filename.ast out_filename.ast headerbytes prebytes postbytes\n", VERSION); exit(1);
    }
    
    headerbytes = atoi(argv[3]);
    prebytes = atoi(argv[4]);
    postbytes = atoi(argv[5]);

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
    
    i+=headerbytes;
    while( i<filesize ) {
//	int j;
	i += prebytes;
	lendb = (ptr[i+1]<<8) + ptr[i+2];
//	printf("%02X %02X len(%d) pre(%d) post(%d)\n", ptr[i+1], ptr[i+2], lendb, prebytes,postbytes);
//	for(j=0;j<lendb+prebytes+postbytes;j++) printf("%02X ", ptr[i+j-prebytes]); printf("\n");
        if (write(fdout, ptr + i, lendb) != lendb) {
	    printf("error write\n"); exit(1);
	}
	i += postbytes + lendb;
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
