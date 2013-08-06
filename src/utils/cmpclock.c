//stty -F /dev/ttyS4 9600 cs7 -cstopb clocal cread
//XGUAAMMDDhhmmss\c013
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>
#include <syslog.h>
#include <paths.h>
#include <string.h>
#include <setjmp.h>
#include "defines.h"

#define READCHAR 16

int main(int argc, char *argv[]) {
unsigned char readbuff[READCHAR+1];
//char parsed[READCHAR+1];
char tmpbuff2[2];
int fd, count,i;
struct tm t;
struct timeval tvread;
struct timeval tvlocal;
    
    printf("cmpclock%s\n", VERSION);

    fd = open("/dev/ttyS0", O_RDONLY);
    if (fd<0) {
	perror("open");  return -1;
    }
    
    tvread.tv_usec = (((float)READCHAR)/(9600.0/9.0))*1000000;
    for (;;) {
//	bzero(readbuff, READCHAR + 1);
//	bzero(parsed, READCHAR);
//	bzero(&t, sizeof(struct tm));
	
	count = read(fd, readbuff, READCHAR);

	for (i=0;i<READCHAR;i++) { printf("%02X ", readbuff[i]); }
	printf("\n");
	//for (i=0;i<READCHAR;i++) { printf("%c\n", readbuff[i]); }
	//printf("\n");

	if (count < READCHAR) {
	    printf("desincronizacion\n");
	}
	
        memcpy(tmpbuff2,readbuff+13,2); t.tm_sec = atoi(tmpbuff2);
	memcpy(tmpbuff2,readbuff+11,2); t.tm_min = atoi(tmpbuff2);
	memcpy(tmpbuff2,readbuff+9,2);  t.tm_hour = atoi(tmpbuff2);
	memcpy(tmpbuff2,readbuff+7,2);  t.tm_mday = atoi(tmpbuff2);
	memcpy(tmpbuff2,readbuff+5,2);  t.tm_mon = atoi(tmpbuff2) - 1;
	memcpy(tmpbuff2,readbuff+3,2);  t.tm_year = atoi(tmpbuff2) + 100;
	
	tvread.tv_sec = mktime(&t); 
	if (tvread.tv_sec < 0)
	    perror("mktime");

	count = gettimeofday(&tvlocal, (struct timezone *) NULL);
	if (count < 0) {
	    perror("gettimeofday\n");
	}
	settimeofday(&tvread, (struct timezone *) NULL);
//	count = strftime(parsed, READCHAR, "%y%m%d%H%M%S", &t);

//	if (count <0)
//	    perror("strftime1");
	    
//	readbuff[READCHAR-1] = '\0';
//	printf("[%s][%s][%ld][%ld]\n", readbuff, parsed, tvread.tv_sec, tvlocal.tv_sec);

	printf("%ld.%06ld %ld.%06ld (%3.6f)\n", tvread.tv_sec, tvread.tv_usec, 
						tvlocal.tv_sec, tvlocal.tv_usec,
						((float)tvread.tv_sec+(float)tvread.tv_usec/1000000) - ((float)tvlocal.tv_sec+(float)tvlocal.tv_usec/1000000));
//	printf("%ld.%06ld\n", tvlocal.tv_sec, tvlocal.tv_usec);
    }
}    

    
	/* loop  until we die */
/*	printf("...starting\n");
	for (;;) {
	    	char buf2[4];
		struct timeval radio;
		struct tm t;
		//arg = WaitOnSerialChange(serial, &tv);
		//usleep(5000*1000);
		bzero(buf,17);
		bzero(&t,sizeof(struct tm));
		bzero(buf2,4);    
		bzero(buf2,4);    
		bzero(buf2,4);
		bzero(buf2,4);
		bzero(buf2,4);
		bzero(buf2,4);
		t.tm_isdst = 0;
    		radio.tv_usec = 0;
	        if ((l = read(serial, &buf, 17)) > 0) {

		    memcpy(buf2,buf+13,2); t.tm_sec = atoi(buf2);
		    memcpy(buf2,buf+11,2); t.tm_min = atoi(buf2);
		    memcpy(buf2,buf+9,2);  t.tm_hour = atoi(buf2);
		    memcpy(buf2,buf+7,2);  t.tm_mday = atoi(buf2);
		    memcpy(buf2,buf+5,2);  t.tm_mon = atoi(buf2) - 1;
		    memcpy(buf2,buf+3,2);  t.tm_year = atoi(buf2)+100;
		    
		    if ((radio.tv_sec = mktime(&t)) < 0) 
			perror("mktime");
		
		    radio.tv_sec += 3600; //599;
//		    radio.tv_usec = 989000;
		    //printf("%ld\n",radio.tv_sec);
		    if (first!=5) {
			if (first<5) {
			    first++;
			}
		    } else {
			settimeofday(&radio,(struct timezone *)NULL);
			first++;
			printf("hard synch...");
		    }

	    	    shmT->mode = 1;
		    shmT->valid = 0;

		    __asm__ __volatile__ ("":::"memory");
		    shmT->leap = 0;
		    shmT->precision = PRECISION;
		    shmT->clockTimeStampSec =  (time_t) radio.tv_sec;
		    shmT->clockTimeStampUSec =  (int) (radio.tv_usec/1000);
		    shmT->receiveTimeStampSec =  (time_t) radio.tv_sec;
		    shmT->receiveTimeStampUSec = (int) (radio.tv_usec/1000);
		    __asm__ __volatile__ ("":::"memory");

		    shmT->count=0;
		    shmT->valid = 1;
	
		    printf("%d:%d:%d %d-%d-%d %s", t.tm_hour, t.tm_min, t.tm_sec, t.tm_mday, t.tm_mon, t.tm_year - 100 + 2000, asctime(&t));
	
		} 
	    
		// now do the same for a clock on the DSR line
//		ProcessStatusChange(&dsr, (arg & TIOCM_DSR), &tv);

		// print pulse information on stdout if in test mode
		/// warn if valid time stamp not received in the last 5 mins
	}
	return 0;
}
*/
