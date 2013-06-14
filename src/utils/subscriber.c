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

#include "includes.h"

// #define READ_GROUP_IP "225.185.210.1"
// #define READ_GROUP_PORT 4001

int main(int argc, char*argv[]) {

int sdr, sdw, yes = 1;
socklen_t size_sock_r,size_sock_w;
struct sockaddr_in sock_r, sock_w;
struct ip_mreq imr;
unsigned char buf[1024];
char READ_GROUP_IP[256];
char READ_IFAZ_IP[256];
int READ_GROUP_PORT;
    
    strncpy(READ_GROUP_IP, argv[1], 255);
    READ_GROUP_PORT = strtol(argv[2], NULL, 10);
    strncpy(READ_IFAZ_IP, argv[3], 255);

    printf("%s:%d %s\n", READ_GROUP_IP, READ_GROUP_PORT, READ_IFAZ_IP);

    sdr = socket(PF_INET, SOCK_DGRAM, 0);
    if (sdr < 0) { perror("socket"); exit(EXIT_FAILURE); }
    
    sdw = socket(PF_INET, SOCK_DGRAM, 0);
    if (sdw < 0) { perror("socket"); exit(EXIT_FAILURE); }

    memset(&sock_r, 0, sizeof(sock_r));
    sock_r.sin_family = AF_INET;
    sock_r.sin_port = htons(READ_GROUP_PORT);
#ifdef SOLARIS
    sock_r.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    sock_r.sin_addr.s_addr = inet_addr(READ_GROUP_IP);
#endif

    /*
     * initialization of the socket of emission
     */
    memset(&sock_w, 0, sizeof(sock_w));
    sock_w.sin_family = AF_INET;
    sock_w.sin_port = htons(READ_GROUP_PORT);
    sock_w.sin_addr.s_addr = htonl(INADDR_ANY);

    size_sock_r = sizeof(sock_r);
    size_sock_w = sizeof(sock_w);

// suscriptoin
    if ( setsockopt(sdr, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt so_reuseaddr"); exit(EXIT_FAILURE);
    }

    if (bind(sdr, (struct sockaddr *) &sock_r, sizeof(sock_r)) < 0) { perror("bind"); exit(EXIT_FAILURE); }
    
    imr.imr_multiaddr.s_addr = inet_addr(READ_GROUP_IP);
    imr.imr_interface.s_addr = inet_addr(READ_IFAZ_IP); // htonl(INADDR_ANY);

    if (setsockopt(sdr, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imr, sizeof(imr)) < 0) {
        perror("setsockopt IP_ADD_MEMBERSHIP"); exit(EXIT_FAILURE); 
    }

    while (1) {
        ssize_t cnt = recvfrom(sdr, buf, sizeof(buf), 0, (struct sockaddr *) &sock_r, &size_sock_r);
        if (cnt < 0) { perror("recvfrom"); exit(EXIT_FAILURE); }
        else 
        if (cnt == 0) { /* end of transmission */
            break;
        }
        printf("%d\n", cnt); /* posting of the message */
    }

    exit(EXIT_SUCCESS);
}
