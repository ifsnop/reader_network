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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "defines.h"
#define LAP_TIME 600
	 
int do_exit=0;
int s_in;
struct ip_mreq mreq;
struct sockaddr_in cast_group;

void gotsig(int sig) {
    sig = sig;                  /* -Wall complains its unused */
    signal(SIGHUP, gotsig);
    do_exit=1;    
}

void subscribe() {
// #route add -net 225.25.250.0 netmask 255.255.255.0 dev eth2
// #route add -net 225.25.250.0 -netmask 255.255.255.0 214.25.250.9 -iface
    
    s_in = socket(PF_INET, SOCK_DGRAM, 0);
    if (s_in<0) {printf("error socket in:%s\n",strerror(errno)); exit(EXIT_FAILURE);}
    memset(&cast_group, 0, sizeof(cast_group));
    cast_group.sin_family = AF_INET;
    cast_group.sin_addr.s_addr = inet_addr("225.25.250.9"); //multicast group ip
    cast_group.sin_port = htons((unsigned short int)5020);  //multicast group port
    if ( bind(s_in, (struct sockaddr *) &cast_group, sizeof(cast_group)) < 0) {
        printf("error bind: %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr = cast_group.sin_addr; //multicast group ip
    if (setsockopt(s_in, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
#ifdef LINUX
        printf("error add_membership: %s\nmissing: #route add -net 225.25.250.0 netmask 255.255.255.0 dev eth2 ? \n", strerror(errno)); exit(EXIT_FAILURE);
#endif
#ifdef SOLARIS
        printf("error add_membership: %s\nmissing: #route add -net 225.25.250.0 -netmask 255.255.255.0 214.25.250.9 -iface ? \n", strerror(errno)); exit(EXIT_FAILURE);
#endif
    }
    return;
}

int main(int argc, char *argv[]) {
    unsigned char dump_start[] = {0x00,0x06,0x00,0x02,0x01,0x01,0x03,0xe8,0x53,0x69,0x52,0x73};
    unsigned char dump_stop[]  = {0x00,0x06,0x00,0x02,0x01,0x01,0x03,0xe8,0x4e,0x6f,0x52,0x73};
    char ptr_destdir[256], ptr_mkdir[256], ptr_bzip[256], *ptr_filename;
//    00,06 -> tamano en words
//    00 -> 
//    02 -> origen msg: ucs
//    01 -> num. de extractor (si es al de la lan2, poner un 2)
//    01 -> num. de extractores a los que se dirige el msg (siempre 1)
//    03,e8 -> codigo del tipo de mensaje
//    53,69,52,73 -> "SiRs" (si queremos respuestas) empezar
//    4e,6f,52,73 -> "NoRs" (no queremos respuestas) parar
//    214.25.250.14:1044 -> 214.25.250.255:5001 UDP (12 bytes)    
//    214.25.250.14:1044 -> 214.25.250.255:2030 UDP (12 bytes)    
    
    int s_out,ret,val=1,fd_out;
    struct sockaddr_in addr1,addr2;
    unsigned char* ptr_in;
    struct timeval current_date,old_date;
    struct tm *t2;    

    printf("memresp %s\n", VERSION);
    
    if (argc<2 || argc>3) { //un parametro
	printf("syntax error: memresp dest_dir [daemon]\ncurrent(%d)\n",argc); exit(EXIT_FAILURE);
    }
   
    if (argc==3 && !strcmp(argv[2],"daemon")) {
        printf("going daemon...\n");
#ifdef LINUX
        daemon(1,0);
#endif
#ifdef SOLARIS
        if (getppid()==1) {
            printf("ERROR getppid (already a daemon): %s\n", strerror(errno)); exit(EXIT_FAILURE);
        }
        if ( (retcode = fork())<0 ) {
	    printf("ERROR fork: %s\n", strerror(errno)); exit(EXIT_FAILURE);
	} else {
	    if (retcode>0) {
		exit(EXIT_SUCCESS);
	    }
	}
	// child (daemon) continues
	setsid(); // obtain a new process group
	// close all open filehandlers
	{
	    int fd = 0; long retcode;
	    if ( (retcode = sysconf(_SC_OPEN_MAX)) <0 ) {
		printf("ERROR sysconf: %s\n", strerror(errno)); exit(EXIT_FAILURE);
	    }
	    while (fd < retcode) close(fd++);
	    retcode = open("/dev/null",O_RDWR); dup(retcode); dup(retcode); // handle standart I/O
	    umask(027); // set newly created file permissions -> 750
	    // chdir("/"); // change running directory  
	}
#endif
    }
 
    ptr_in = (unsigned char*)malloc(65535);
    memset(ptr_in, 0x00, 65535);	    
    ptr_filename = (char*)malloc(256);
    memset(ptr_filename, 0x00, 256);

// captura de senyales para salir limpiamente (mandando un stop al volcado de resp
    signal(SIGHUP,gotsig);signal(SIGINT,gotsig);
    signal(SIGKILL,gotsig);signal(SIGSTOP,gotsig);
    signal(SIGTERM,gotsig);
    
// envio del paquete para empezar el volcado
    s_out = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_out<0) {printf("error socket out:%s\n",strerror(errno)); exit(EXIT_FAILURE);}
    
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(2030); // puerto
    addr1.sin_addr.s_addr = inet_addr("214.25.250.255"); // ip destino
    addr2.sin_family = AF_INET;
    addr2.sin_port = htons(5001); // puerto
    addr2.sin_addr.s_addr = inet_addr("214.25.250.255"); // ip destino
    ret = setsockopt(s_out, SOL_SOCKET, SO_BROADCAST, (const char*)&val, sizeof(val)); // obligado para escribir en broadcast
    if (ret<0) {printf("error setsockopt(%d):%s\n", ret, strerror(errno)); exit(EXIT_FAILURE);}
    ret = sendto(s_out, dump_start, 12, 0, (struct sockaddr*)&addr1, sizeof(addr1));
    if (ret!=12) {printf("error sendto(%d):%s\n", ret, strerror(errno)); exit(EXIT_FAILURE);}
    ret = sendto(s_out, dump_start, 12, 0, (struct sockaddr*)&addr2, sizeof(addr2));
    if (ret!=12) {printf("error sendto(%d):%s\n", ret, strerror(errno)); exit(EXIT_FAILURE);}

    subscribe();

// sacar la fecha actual para el nombre del fichero
    gettimeofday(&current_date, NULL);
    current_date.tv_sec = old_date.tv_sec = current_date.tv_sec - (current_date.tv_sec % LAP_TIME);
    t2 = gmtime(&current_date.tv_sec);

    sprintf(ptr_destdir, "%s/%02d/", argv[1], t2->tm_mday);
    sprintf(ptr_mkdir, "/bin/mkdir -p %s", ptr_destdir);
    if ((ret = system(ptr_mkdir)) < 0) {
       printf("error mkdir: %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    sprintf(ptr_filename, "%s%02d%02d%02d.bin", ptr_destdir, t2->tm_hour, t2->tm_min, t2->tm_sec);
    printf(">%s\n", ptr_filename);
	
    fd_out = open(ptr_filename, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR
                      | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd_out<0) { printf("error open(1): %s (%s)\n", strerror(errno), ptr_filename); exit(EXIT_FAILURE); }

    while(do_exit==0) { 
	int select_count = 0;
	struct timeval timeout;
	fd_set reader_set;

	timeout.tv_sec = 2; timeout.tv_usec = 0;
	
	FD_ZERO(&reader_set);
	FD_SET(s_in, &reader_set);
	
///
	select_count = select(s_in+1, &reader_set, NULL, NULL, &timeout);
///

	gettimeofday(&current_date, NULL);
	current_date.tv_sec -= (current_date.tv_sec % LAP_TIME);
	if (current_date.tv_sec >= (old_date.tv_sec + LAP_TIME)) {
	    close(fd_out);
	    // compressing old memresp recording in background
	    sprintf(ptr_bzip, "/bin/sh -c '/bin/bzip2 %s' &", ptr_filename);
	    if ( (ret = system(ptr_bzip)) < 0 ) {
                printf("error bzip2: %s\n", ptr_bzip); exit(EXIT_FAILURE);
	    }
	    t2 = gmtime(&current_date.tv_sec);
            sprintf(ptr_destdir, "%s/%02d/", argv[1], t2->tm_mday);
            sprintf(ptr_mkdir, "/bin/mkdir -p %s", ptr_destdir);
            if ((ret = system(ptr_mkdir)) < 0) {
               printf("error mkdir: %s\n", strerror(errno)); exit(EXIT_FAILURE);
            }
            sprintf(ptr_filename, "%s%02d%02d%02d.bin", ptr_destdir, t2->tm_hour, t2->tm_min, t2->tm_sec);

	    fd_out = open(ptr_filename, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	    if (fd_out<0) { printf("error open(2): %s (%s)\n", strerror(errno), ptr_filename); exit(EXIT_FAILURE); }
	    printf(">%s\n", ptr_filename);
	    old_date.tv_sec = current_date.tv_sec;
	}

	if (select_count>0) {
	    if (FD_ISSET(s_in, &reader_set)) {
		int udp_size=0;
		socklen_t addrlen = sizeof(struct sockaddr_in);
	    	memset(&cast_group, 0, sizeof(cast_group));
	        if ((udp_size = recvfrom(s_in, ptr_in, 65535, 0, (struct sockaddr *) &cast_group, &addrlen)) < 0) {
		    printf("error recvfrom: %s\n", strerror(errno)); //exit(EXIT_FAILURE);
                    close(s_in);
                    subscribe();
        	}
		if (( udp_size > 0) && (!strcasecmp(inet_ntoa(cast_group.sin_addr), "214.25.250.1"))) {
		    // write to file
                    ret = write(fd_out, ptr_in, udp_size);
                    if ( ret < 0 ) {
                        printf("error write: %s\n", strerror(errno)); exit(EXIT_FAILURE);
                    }
		} else {
		    // no sabemos de donde viene!
		}
	    }
	} else {
	    if (select_count<0) {
		printf("error select: %s\n", strerror(errno));
	    } else {
//		printf("timeout %d\n", timeout.tv_sec);
	    }
	}
//	if (do_exit) { printf("salir\n"); }
    }    
//    printf("salido\n");
    
    ret = sendto(s_out, dump_stop, 12, 0, (struct sockaddr*)&addr1, sizeof(addr1));
    if (ret<12) { printf("error sendto(%d): %s\n", ret, strerror(errno)); exit(EXIT_FAILURE); }
    ret = sendto(s_out, dump_stop, 12, 0, (struct sockaddr*)&addr2, sizeof(addr2));
    if (ret<12) { printf("error sendto(%d): %s\n", ret, strerror(errno)); exit(EXIT_FAILURE); }

    close(fd_out);
    close(s_in);
    close(s_out);

    exit(EXIT_SUCCESS);
}
