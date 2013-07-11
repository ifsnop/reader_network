/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2013 Diego Torres <diego dot torres at gmail dot com>

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
#include <math.h> 
#include "defines.h"
#include "crc32.h"
#include "red_black_tree.h"
	 
// rb_red_blk_tree* tree = NULL;
struct Queue {
    rb_red_blk_node **node;
    //[MAX_SCRM_SIZE];
    int front, rear;
    int count;
};

struct Queue q;
int maxModeA = 0;

void AddQueue(void* a) {
    // printf("add addr:%x crc:%x\n", (unsigned int)a, ((rb_red_blk_node*)a)->crc32);
    if (q.count != MAX_SCRM_SIZE) { q.node[q.rear] = (rb_red_blk_node *) a; q.rear = (q.rear + 1) % MAX_SCRM_SIZE; q.count++; }
}
									
void DeleteQueue(void *a) {
    // printf("delete addr:%x crc:%x\n", (unsigned int)a, ((rb_red_blk_node*)a)->crc32);
    // item = queue.node[queue.front];
    if (q.count != 0) { q.front = (q.front + 1) % MAX_SCRM_SIZE; q.count--; }
}
int UIntComp(unsigned int a, unsigned int b) { if (a>b) return (1); if (a<b) return (-1); return 0; }

int main(int argc, char *argv[]) {
    int prebytes=0, postbytes=0, headerbytes=0;
    int fdin, fdout;
    int i=0,j=0;
    unsigned int lendb, filesize;
    unsigned char *ptr;
    unsigned char *ptrDest,*ptrFSPECDest;
    int destcount=0,lendbdest=0;
    rb_red_blk_tree* tree = NULL;
    rb_red_blk_node * node;
    
    if(argc!=6) { printf("convert %s in_filename.ast out_filename.ast headerbytes prebytes postbytes\n", VERSION); exit(1); }
    
    headerbytes = atoi(argv[3]);
    prebytes = atoi(argv[4]);
    postbytes = atoi(argv[5]);

    if ( (fdin = open(argv[1], O_RDONLY)) == -1 ) { printf("error input file\n"); exit(1); }
    if ( (fdout = open(argv[2], O_CREAT|O_WRONLY|O_TRUNC, 0666)) == -1 ) { printf("error output file\n"); exit(1); }
    if ( (filesize = lseek(fdin, 0, SEEK_END)) == -1 ) { printf("error lseek_end file\n"); exit(1); }
    if ( (lseek(fdin, 0, SEEK_SET)) != 0 ) { printf("error lseek_set file\n"); exit(1); }
    if ( (ptr = (unsigned char *) malloc (filesize)) == NULL ) { printf("error malloc\n"); exit(1); }
    if ( (ptrDest = (unsigned char *) malloc (1024)) == NULL ) { printf("error malloc\n"); exit(1); }
    if ( (ptrFSPECDest = (unsigned char *) malloc (1024)) == NULL ) { printf("error malloc\n"); exit(1); }
    if ( read(fdin, ptr, filesize) != filesize ) { printf("error read\n"); exit(1); } 
    
    tree = RBTreeCreate(UIntComp,AddQueue,DeleteQueue);
    q.node = (rb_red_blk_node **) malloc(sizeof(rb_red_blk_node *) * MAX_SCRM_SIZE);
    q.rear = q.front = q.count = 0;

    i+=headerbytes;
    while( i<filesize ) {
	bzero(ptrDest,1024);
	bzero(ptrFSPECDest,1024);

	if (prebytes) { printf(">"); for(j=0;j<prebytes;j++) { printf("%02X ", ptr[i+j]); }; printf("\n"); }

	i += prebytes; lendb = ptr[i+1]*256 + ptr[i+2];

	{
	    int pfspec = i+3, sfspec = 0;
	    int FSPECDestSize = 0;
	    int modeA = 0;
	    j=0;
	    while( (ptr[i+j+3] & 1) == 1) j++; sfspec = j+1;
	    
	    if ( (ptr[pfspec] & 192) == 192) { //FSPEC: is SACSIC + MessageType

		//for(j=0;j<(sfspec+2+3 /*3 bytes from cat, length*/);j++) printf ("   "); printf("^(%d)\n",sfspec);
	    
		if ( (ptr[pfspec+sfspec+2 /*fspec + size fspec + sac sic */] & 3) == 1) { // MessageType = PSR
		    float dst = 0; int idst; //,azi = 0; , destfspecptr = 0;
		    float timestamp = 0;
		    if ( (ptr[pfspec] & 4) != 4)  // DI040, si no existe, no nos interesa este reporte (ES DE FIN DE PISTA)
			goto siguiente;

		    printf("P>"); for(j=0;j<lendb;j++) { printf("%02X ", ptr[i+j]); }; if (lendb) printf("\n");
		    ptrFSPECDest[0] = 0x30; 					// CAT 48
		    ptrFSPECDest[1] = 0x00; 					// SIZE1 0
		    ptrFSPECDest[2] = 0x0a; lendbdest = 10; 			// SIZE2 
		    ptrFSPECDest[3] = 0xF0; FSPECDestSize = 1; 			// FSPEC
		    
		    ptrDest[destcount] = ptr[pfspec+sfspec]; destcount++;   	// SAC
		    ptrDest[destcount] = ptr[pfspec+sfspec+1]; destcount++; 	// SIC
		    ptrDest[destcount] = ptr[pfspec+sfspec+4]; destcount++; 	// UTC1 HOUR
		    ptrDest[destcount] = ptr[pfspec+sfspec+5]; destcount++; 	// UTC2 HOUR
		    ptrDest[destcount] = ptr[pfspec+sfspec+6]; destcount++; 	// UTC3 HOUR
		    timestamp = (((ptr[pfspec+sfspec+4]) << 16) +
			((ptr[pfspec+sfspec+5]) << 8) +
			(ptr[pfspec+sfspec+5]))/128.0;
		    ptrDest[destcount] = 0x40; destcount++; 			// TARGET REPORT DESCRIPTION 
										// PSR0x20, SSR0x40, CMB0x60
		    dst = (((ptr[pfspec+sfspec+7]<<8) + (ptr[pfspec+sfspec+8]))/1852.0)*256.0;
		    idst = (int)floor(dst);
		    //printf("%02X __ %d %d %d\n",(ptr[pfspec+sfspec+7]<<8) + (ptr[pfspec+sfspec+8]),idst, idst & 0xff00, idst & 0xff);

		    ptrDest[destcount] = (idst & 0xff00) >> 8; destcount++; 	// DI040 RHO
		    ptrDest[destcount] = (idst & 0xff); destcount++; 		// DI040 RHO LSB
		    ptrDest[destcount] = ptr[pfspec+sfspec+9]; destcount++; 	// DI040 THETA
		    ptrDest[destcount] = ptr[pfspec+sfspec+10]; destcount++; 	// DI040 THETA LSB

		    printf("W>"); for(j=0;j<3 + FSPECDestSize;j++) { printf("%02X ", ptrFSPECDest[j]); }; printf("."); for(j=0;j<lendbdest;j++) { printf("%02X ", ptrDest[j]); }; if (lendbdest) printf("\n");
		    if ( (ptr[pfspec] & 1) == 1 ) { 				// si hay 2 bytes de fpsec
			if ( (ptr[pfspec + 1] & 32) == 32) { 			// y hay DI161
			    {   
				// MOMENTO CHURRO, vamos a convertir el track number en un modo A!
				// vamos a usar crc como ModoA y access como pista cancelada (1=cancelada, 0=viva)
				int trackNumber = ((ptr[pfspec+sfspec+23]<<8) + ptr[pfspec+sfspec+24]);
				if ( (node = RBExactQuery(tree, trackNumber)) == NULL ) { // si no existe
				    if (tree->count>=MAX_SCRM_SIZE) { // si maximo alcanzado
					rb_red_blk_node *node_old = q.node[q.front];
					RBDelete(tree, node_old); // se borra el mas antiguo
				    }
				    maxModeA++;
				    modeA = maxModeA;
				    node = RBTreeInsert(tree,trackNumber,timestamp);
				    node->access = modeA;
				} else {
				    if (timestamp < ((node->timestamp)+15 ) ) {
					modeA = node->access;
				    } else { // demasiado antiguo, asigna otro codigo
					maxModeA++;
					node->access = modeA = maxModeA;
				    }
				    node->timestamp = timestamp;
				}
				//RBTreePrint(tree);
				//printf("TRACK#(%d) MODEA(%o)\n", modeA, modeA & 0x7777);
				ptrDest[destcount] = (modeA & 0xff00) >> 8; destcount++;// DI070 MODE3A
				ptrDest[destcount] = (modeA & 0xff); destcount++;	// DI070 MODE3A LSB
				lendbdest += 2;
				ptrFSPECDest[3] |= 8; // DI070
			      // YA QUE ESTAMOS, LE ASIGNAMOS UNA ALTURA DE 1000 pies   // DI090
			        ptrDest[destcount]= 0x00; destcount++;			// DI090 MODEC
				ptrDest[destcount] = 10*4; destcount++; // 1000 pies, FL10, como LSB es 1/4
				lendbdest += 2;
				ptrFSPECDest[3] |= 4; 					// DI090
			    }

			    ptrDest[destcount] = ptr[pfspec+sfspec+23]; destcount++; 	// DI161 TRACK NUMBER
			    ptrDest[destcount] = ptr[pfspec+sfspec+24]; destcount++; 	// DI161 TRACK NUMBER LSB
			    lendbdest += 2; 						// 2 del DI161
			    ptrFSPECDest[3] |= 1; 					// FX=1
			    ptrFSPECDest[4] |= 16;  					// DI161
			    if (FSPECDestSize<2) FSPECDestSize++;
			    
			}
			//printf("%d %02x\n",(ptr[pfspec+1] & 128)==128,ptr[pfspec+1]);
			if ( (ptr[pfspec + 1] & 128) == 128) { 				// y hay DI200
			    ptrDest[destcount] = ptr[pfspec+sfspec+15]; destcount++; 	// DI200 GROUND SPEED
			    ptrDest[destcount] = ptr[pfspec+sfspec+16]; destcount++; 	// DI200 GROUND SPEED LSB
			    ptrDest[destcount] = ptr[pfspec+sfspec+17]; destcount++; 	// DI200 TRACK ANGLE
			    ptrDest[destcount] = ptr[pfspec+sfspec+18]; destcount++; 	// DI200 TRACK ANGLE LSB
			    lendbdest += 4; 						// 4 del DI200
			    ptrFSPECDest[3] |= 1; 					// FX=1
			    ptrFSPECDest[4] |= 4;  					// DI200
			    if (FSPECDestSize<2) FSPECDestSize++;
			}
			if ( (ptr[pfspec + 1] & 16) == 16) { 				// y hay DI170
			    ptrDest[destcount] = ptr[pfspec+sfspec+25]; destcount++; 	// DI170 TRACK STATUS 1
			    if ( (ptr[pfspec+sfspec+25] & 64) == 64 ) {			// TRE = 1 (ultimo mensaje de pista)
				if ( (node = RBExactQuery(tree, modeA)) != NULL ) { // si no existe
				    node->access = 1;
				} else {
				    printf("ERROR: mensaje de fin de pista sin track number\n");
				}
			    }
			    lendbdest++; // 2 del DI170
			    if ( (ptr[pfspec+sfspec+25] & 1) ==1 ) {			// TRACK STATUS HAS FX
				ptrDest[destcount] = ptr[pfspec+sfspec+26]; destcount++;// DI170 TRACK STATUS 2
				lendbdest++; 						// 2 del DI170
			    }
			    if ( (ptr[pfspec+sfspec+26] & 1) ==1 ) {			// TRACK STATUS HAS FX
				ptrDest[destcount] = ptr[pfspec+sfspec+27]; destcount++;// DI170 TRACK STATUS 3
				lendbdest++; 						// 2 del DI170
			    }

			    ptrFSPECDest[3] |= 1; 					// FX=1
			    ptrFSPECDest[4] |= 2; 					// DI170
			    if (FSPECDestSize<2) FSPECDestSize++;
			}
			
		    }
		    
		    printf("W>"); for(j=0;j<3 + FSPECDestSize;j++) { printf("%02X ", ptrFSPECDest[j]); }; printf("."); for(j=0;j<lendbdest;j++) { printf("%02X ", ptrDest[j]); }; if (lendbdest) printf("\n");
		    memcpy(ptrFSPECDest+FSPECDestSize+3, ptrDest, lendbdest);
		    lendbdest+=FSPECDestSize+3;
		    ptrFSPECDest[2]=lendbdest;
		    memcpy(ptrDest,ptrFSPECDest,lendbdest); // se copia dos veces porque lo que se guarda en fichero esta en ptrDest
//		    printf("W>"); for(j=0;j<lendbdest;j++) { printf("%02X ", ptrDest[j]); }; if (lendbdest) printf("\n");		    
		}
		
	    
		if ( (ptr[pfspec+sfspec+2 /*fspec + size fspec + sac sic */] & 3) == 2 ) { //MessageType = NORTH_CROSSING
		    //printf("N>"); for(j=0;j<lendb;j++) { printf("%02X ", ptr[i+j]); }; if (lendb) printf("\n");    
		    //printf("NORTH %02X\n",ptr[pfspec+sfspec+2]);
		    ptrDest[destcount] = 0x22; destcount++; 				// CAT 34
		    ptrDest[destcount] = 0x00; destcount++; 				// SIZE1 0
		    ptrDest[destcount] = 0x0a; destcount++; lendbdest=10; 		// SIZE2 10
		    ptrDest[destcount] = 0xe0; destcount++; 				// FSPEC
		    ptrDest[destcount] = ptr[pfspec+sfspec]; destcount++; 		// SAC
		    ptrDest[destcount] = ptr[pfspec+sfspec+1]; destcount++; 		// SIC
		    
		    ptrDest[destcount] = 0x01; destcount++; 				// MSG TYPE = NORTH CROSS
		    ptrDest[destcount] = ptr[pfspec+sfspec+3]; destcount++; 		// UTC1 HOUR
		    ptrDest[destcount] = ptr[pfspec+sfspec+4]; destcount++; 		// UTC2 HOUR
		    ptrDest[destcount] = ptr[pfspec+sfspec+5]; destcount++; 		// UTC3 HOUR
		    //for(j=destcount - lendbdest;j<destcount;j++) { printf("%02X ", ptrDest[j]); }; if (lendbdest) printf("\n");
		    //printf("W>"); for(j=0;j<lendbdest;j++) { printf("%02X ", ptrDest[j]); }; if (lendbdest) printf("\n");
		}
	    }
	}
	
	if (postbytes) { printf(">"); for(j=0;j<postbytes;j++) { printf("%02X ", ptr[i+j+lendb]); }; printf("\n"); }
	

	
	
//	printf("%02X %02X %d\n", ptr[i+1], ptr[i+2], lendb);
//	for(j=0;j<lendb+prebytes+postbytes;j++) printf("%02X ", ptr[i+j-prebytes]); printf("\n");
	if (lendbdest>0) {
    	    if (write(fdout, ptrDest, lendbdest) != lendbdest) {
		printf("error write\n"); exit(1);
	    }
//	    printf("W\n");
	    printf("---\n");
	}
siguiente:	
	i += postbytes + lendb;
	destcount=0;lendbdest=0;
    }

    free(ptr);
    free(ptrDest);
    free(ptrFSPECDest);
    
    if (close(fdout) == -1) {
	printf("error close fdout\n"); exit(1);
    }
    if (close(fdin) == -1) {
	printf("error close fdin\n"); exit(1);
    }
    exit(EXIT_SUCCESS);
}
