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

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
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

#define READ_MCAST "225.25.250.1"
#define READ_PORT 4002
#define READ_IFACE "214.25.250.7"

#define WRITE_MCAST1 "225.25.250.1"
#define WRITE_PORT1 4099
#define WRITE_IFACE1 "214.25.250.7"

#define WRITE_MCAST2 "225.25.249.1"
#define WRITE_PORT2 4099
#define WRITE_IFACE2 "214.25.249.7"

#define MAX_UDP_SIZE 65535

char *parse_hora(double segs) {
    char tmp[8];
    char *res;
    float tmpsecs;
    struct tm *stm;
    time_t t;

    res = (char*) malloc(14);
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

void print_packet(char *pre, unsigned char * ptr, int k, int size) {
    struct timeval t;
    double msecs = 0.0,secs = 0.0;
    int i;

    if ( gettimeofday(&t, NULL) == -1 ) {
        printf("ERROR gettimeofday: %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
        
    msecs = t.tv_usec/1000000.0; 
    secs = t.tv_sec + msecs;

    printf("%s>%03d %s] ", pre, k, parse_hora(secs));
    
    for(i=0; i<size; i++)
	printf("%02X ", ptr[i]); 
    printf("\n"); 

    return;
}


int main(int argc, char *argv[]) {

    struct hostent *h;
    struct sockaddr_in cliaddr, cast_group;
    struct sockaddr_in srvaddr2, srvaddr3;
    struct in_addr inp;
    struct ip_mreq mreq;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    int s1, s2, s3, udp_size, i, yes = 1, loop = 1;
    unsigned char ttl = 32;
    unsigned char *ptr;

    if ( (h = gethostbyname(WRITE_MCAST1)) == NULL ) {
        printf("ERROR gethostbyname: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&mreq, 0, sizeof(struct ip_mreq));
    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    memset(&srvaddr2, 0, sizeof(struct sockaddr_in));
    srvaddr2.sin_family = h->h_addrtype;
    memcpy((char*) &srvaddr2.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    srvaddr2.sin_port = htons(WRITE_PORT1);
    if ( (s2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("ERROR socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    cliaddr.sin_family = AF_INET;
    inet_aton(WRITE_IFACE1, &inp);
    cliaddr.sin_addr.s_addr = inp.s_addr;
    cliaddr.sin_port = htons(WRITE_PORT1);
    if ( bind(s2, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) <0 ) {
        printf("ERROR bind(s2): %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    setsockopt(s2, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    setsockopt(s2, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    if ( (h = gethostbyname(WRITE_MCAST2)) == NULL ) {
        printf("ERROR gethostbyname: %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
    memset(&srvaddr3, 0, sizeof(struct sockaddr_in));
    srvaddr3.sin_family = h->h_addrtype;
    memcpy((char*) &srvaddr3.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
    srvaddr3.sin_port = htons(WRITE_PORT2);
    if ( (s3 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("ERROR socket(s3): %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    cliaddr.sin_family = AF_INET;
    inet_aton(WRITE_IFACE2, &inp);
    cliaddr.sin_addr.s_addr = inp.s_addr;
    cliaddr.sin_port = htons(WRITE_PORT2);
    if ( bind(s3, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) <0 ) {
        printf("ERROR bind(s3): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    setsockopt(s3, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    setsockopt(s3, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    if ( (s1 = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	printf("ERROR socket(s1) ( %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }

    if ( setsockopt(s1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        printf("ERROR setsockopt reuseaddr(s1): %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    if ( setsockopt(s1, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        printf("ERROR setsockopt ip_multicast_ttl(s1): %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }

    memset(&cast_group, 0, sizeof(cast_group));
    cast_group.sin_family = AF_INET;
    cast_group.sin_addr.s_addr = inet_addr(READ_MCAST);
    cast_group.sin_port = htons(READ_PORT);
    if ( bind(s1, (struct sockaddr *) &cast_group, sizeof(cast_group)) < 0) {
        printf("ERROR bind(s1): %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }

    if ( (mreq.imr_interface.s_addr = inet_addr(READ_IFACE)) == INADDR_NONE ) {
        printf("ERROR inet_addr(READ_IFACE): %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    if ( (mreq.imr_multiaddr.s_addr = inet_addr(READ_MCAST)) == INADDR_NONE ) {
        printf("ERROR inet_addr(READ_MCAST): %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }
    
    if (setsockopt(s1, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        printf("ERROR setsockopt add_membership(s): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ( (ptr = (unsigned char *) malloc(MAX_UDP_SIZE)) == NULL ) {
        printf("ERROR malloc: %s\n", strerror(errno)); exit(EXIT_FAILURE);
    }

    memset(ptr, 0, MAX_UDP_SIZE);

    // iterate 
    for(i=0;;i++) {
	if ((udp_size = recvfrom(s1, ptr, 65535, 0, (struct sockaddr *) &cast_group, &addrlen)) < 0) {
	    printf("ERROR recvfrom: %s\n", strerror(errno)); exit(EXIT_FAILURE);
	}

	print_packet("inp", ptr, i, udp_size); // before modification

	// ensure that only simple packets are modified
	if ( (ptr[0] == 0x01) && (ptr[1] == 0x00) && (ptr[2] == 0x12) && (ptr[3] == 0xfe) &&
	    (ptr[4] == 0x14) && (ptr[5] == 0x01) && (ptr[6] == 0x20) ) {
	    // sac = 0x14, sic = 0x01

	    // modification of mode a code, only if code a matches
	    if ( (ptr[11]==0x09) && (ptr[12]==0x89) ) {
		
		// several mode a codes that could indicate hijack if badly decoded
		ptr[11] = 0x00; ptr[12] = 0x2f; //0057
		ptr[11] = 0x00; ptr[12] = 0x37; //0067
		ptr[11] = 0x0f; ptr[12] = 0x80; //7600
		ptr[11] = 0x0f; ptr[12] = 0xc0; //7700
		ptr[11] = 0x00; ptr[12] = 0x3f; //0077
		ptr[11] = 0x00; ptr[12] = 0x5f; //0137
		ptr[11] = 0x00; ptr[12] = 0xdf; //0337
	
		printf(">C<\n");
	    }

	    // force garble bit of mode a
	    ptr[11] |= 0x40;
	    // force garble bit of mode c
	    ptr[13] |= 0x40;
	}

	print_packet("out", ptr, i, udp_size); // after modification

        if ( (udp_size = sendto(s2, ptr, udp_size, 0, (struct sockaddr *) &srvaddr2, sizeof(srvaddr2))) < 0) {
		printf("ERROR sendto: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
        }

        if ( (udp_size = sendto(s3, ptr, udp_size, 0, (struct sockaddr *) &srvaddr3, sizeof(srvaddr3))) < 0) {
		printf("ERROR sendto: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
    }

    exit(EXIT_SUCCESS);

}
