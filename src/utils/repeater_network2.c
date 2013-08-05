/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2012 Diego Torres <diego dot torres at gmail dot com>

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

#include <string.h>
#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>

char *parse_hora(double segs) {
char tmp[8];
char *res;
float tmpsecs;

struct tm *stm;
time_t t;

    res = (char*)malloc(14);
    memset(tmp,0x0,8);
    t = (long) floor(segs);
    stm = gmtime(&t);
    strftime(tmp, 7, "%H:%M:", stm);
    if ( (tmpsecs = (float) stm->tm_sec + (segs - floor(segs)) ) < 10.0 )
        sprintf(res, "%s0%1.3f", tmp, tmpsecs);
    else
        sprintf(res, "%s%2.3f", tmp, tmpsecs);

    return res;
};



int main(int argc, char *argv[]) {

    struct hostent *h;
    int loop=1;
    struct sockaddr_in cliaddr;
    struct sockaddr_in srvaddr2;
    struct sockaddr_in srvaddr3;
    struct sockaddr_in cast_group;
    int s2,s3,s;
    int udp_size, j, k;
    int i;
    int yes = 1;
    unsigned char ttl = 32;
    unsigned char *ptr;
    struct ip_mreq mreq;
    struct timeval t;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    double msecs = 0.0;
    double secs = 0.0;
    struct in_addr inp;

    // writer 210
    if ( (h = gethostbyname("225.25.250.1")) == NULL ) {
        printf("ERROR gethostbyname: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    memset(&srvaddr2, 0, sizeof(struct sockaddr_in));
    srvaddr2.sin_family = h->h_addrtype;
    memcpy((char*) &srvaddr2.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    srvaddr2.sin_port = htons(4004);
    if ( (s2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("ERROR socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    cliaddr.sin_family = AF_INET;
    //cliaddr.sin_addr.s_addr = inet_addr("214.25.250.14"); //htonl(INADDR_ANY);
    inet_aton("214.25.250.14", &inp);
    cliaddr.sin_addr.s_addr = inp.s_addr;
    cliaddr.sin_port = htons(4004);
    if ( bind(s2, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) <0 ) {
        printf("ERROR socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    setsockopt(s2, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    setsockopt(s2, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    // writer 209
    if ( (h = gethostbyname("225.25.249.1")) == NULL ) {
        printf("ERROR gethostbyname: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    memset(&srvaddr3, 0, sizeof(struct sockaddr_in));
    srvaddr3.sin_family = h->h_addrtype;
    memcpy((char*) &srvaddr3.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    srvaddr3.sin_port = htons(4004);
    if ( (s3 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("ERROR socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    cliaddr.sin_family = AF_INET;
//    cliaddr.sin_addr.s_addr = inet_addr("214.25.249.14"); //htonl(INADDR_ANY);
    inet_aton("214.25.249.14", &inp);
    cliaddr.sin_addr.s_addr = inp.s_addr;
    cliaddr.sin_port = htons(4004);
    if ( bind(s3, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) <0 ) {
        printf("ERROR socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    setsockopt(s3, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    setsockopt(s3, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

//listo para escribir;

    if ( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	printf("socket reader %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        printf("reuseaddr setsockopt reader %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if ( setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        printf("ip_multicast_ttl setsockopt reader %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&cast_group, 0, sizeof(cast_group));
    cast_group.sin_family = AF_INET;
    cast_group.sin_addr.s_addr = inet_addr("225.25.250.1");//radar_definition[i*5+1]); //htonl(INADDR_ANY);
    cast_group.sin_port = htons(4002); //(unsigned short int)strtol(4001 + 2], NULL, 0)); //multicast group port
    if ( bind(s, (struct sockaddr *) &cast_group, sizeof(cast_group)) < 0) {
        printf("bind reader %s\n", strerror(errno));
	exit(EXIT_FAILURE);
    }

    mreq.imr_interface.s_addr = inet_addr("214.25.250.14"); //radar_definition[i*5 + 4]); //htonl(INADDR_ANY); 
    mreq.imr_multiaddr.s_addr = inet_addr("225.25.250.1");
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        printf("add_membership setsockopt reader: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    ptr = (unsigned char *) malloc(65535);
    memset(ptr,0,65535);

for(k=0;k<6000;k++) {
    for(j=0;j<3;j++) {
	if ((udp_size = recvfrom(s, ptr, 65535, 0, (struct sockaddr *) &cast_group, &addrlen)) < 0) {
	    printf("ERROR recvfrom: %s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	gettimeofday(&t, NULL);
	msecs = t.tv_usec/1000000.0; //((double)t.tv_sec);// + ((double)t.tv_usec)/(double)100000.0;
	secs = t.tv_sec + msecs;

	printf("%03d %s] ", k, parse_hora(secs));
	for(i=0;i<udp_size;i++) { printf("%02X ", ptr[i]); } //printf(" ");

	if (j==0 || j==2) { 
	    if ( (udp_size = sendto(s2, ptr, udp_size, 0, (struct sockaddr *) &srvaddr2, sizeof(srvaddr2))) < 0) {
		printf("ERROR sendto: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	    }
	    printf("L1");
	}
	if (j==1 || j==2) {
//	    int p = (int)rand()/40000000.0;
//	    if (p<udp_size && p > 5) { ptr[p]=p; printf("BING!"); }
//	    printf("[%d]", p);
	    
	    if ( (udp_size = sendto(s3, ptr, udp_size, 0, (struct sockaddr *) &srvaddr3, sizeof(srvaddr3))) < 0) {
		printf("ERROR sendto: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	    }
	    printf("L2");
	}
	printf("\n");
    }

}
    exit(EXIT_SUCCESS);
}
