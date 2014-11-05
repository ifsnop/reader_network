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


struct datablock_plot {
    unsigned char sac; 			//	CAT01 CAT02 CAT08 CAT10
    unsigned char sic;			//	CAT01 CAT02 CAT08 CAT10
    int cat; 				// 	 // categoria original de la info
    int type; 				//	CAT01 CAT02 CAT08  CAT10 CAT34 CAT48 // psr-ssr-cmb-track/pasonorte.../sop-eop.../
    int plot_type; 			//	CAT10
    unsigned int track_plot_number; 	//	CAT01
    float rho;				//	CAT01 CAT48
    float theta;			//	CAT01 CAT48
    float x;                            //      CAT20
    float y;                            //      CAT20
    int modea;				//	CAT01
    int modec;				//	CAT01
    int modec_status;			//	CAT01
    int modea_status;			//	CAT01
    //char modes[7];			//	CAT48
    int modes_address;                  //      CAT48
    char aircraft_id[9];                //      CAT048
    int modes_status;			//	CAT48
    int radar_responses;		//	CAT01
    int available;			//	CAT01
    int flag_test;			//	CAT01 plot/track (si es blanco de test, = 1) CAT10 CAT21
    int flag_ground;			//	CAT10 CAT21
    int flag_sim;			//	CAT10 CAT21
    int flag_fixed;			//	CAT10 CAT21
    float truncated_tod;		//	CAT01
    float tod;				//	CAT01 CAT02 CAT08 CAT10
    float tod_stamp;			//	CAT01 CAT02 CAT08 CAT10
    unsigned long id;			// 	id
    unsigned long index;		//	index if available
};

