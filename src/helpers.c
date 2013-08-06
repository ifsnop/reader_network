#include "includes.h"

inline char *parse_hora(float segs) {
char tmp[8];
char *res;
float tmpsecs;
	    
struct tm *stm;
time_t t;
		    
    res = mem_alloc(14);    
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
