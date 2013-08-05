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
