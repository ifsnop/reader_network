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
    union {
        float rho;			//	CAT01 CAT48
        float x;                        //      CAT20
    };
    union {
        float theta;			//	CAT01 CAT48
        float y;                        //      CAT20
    };
    int modea;				//	CAT01
    int modec;				//	CAT01
    int modec_status;			//	CAT01
    int modea_status;			//	CAT01
    int available;			//	CAT01 CAT48

    union {                                 //      CAT48
        struct {
            int modes_address;                  //      CAT48
            unsigned char aircraft_id[9];       //      CAT48
            unsigned char di048_230_com;        //      CAT48
            unsigned char di048_230_stat;       //      CAT48
            unsigned char di048_230_si;         //      CAT48
            unsigned char di048_230_mssc;       //      CAT48
            unsigned char di048_230_arc;        //      CAT48
            unsigned char di048_230_aic;        //      CAT48
            unsigned char di048_230_b1a;        //      CAT48
            unsigned char di048_230_b1b;        //      CAT48
            unsigned char bds_available;        //      CAT48
            unsigned char bds_10[7];            //      CAT48
            unsigned char bds_17[7];            //      CAT48
            unsigned char bds_30[7];            //      CAT48
            int modes_status;			//	CAT48
        };
        struct {
            int radar_responses;		//	CAT01
            int plot_type; 			//	CAT10
            unsigned int track_plot_number; 	//	CAT01
            int flag_test;			//	CAT01 plot/track (si es blanco de test, = 1) CAT10 CAT21
            int flag_ground;			//	CAT10 CAT21
            int flag_sim;			//	CAT10 CAT21
            int flag_fixed;			//	CAT10 CAT21
            float truncated_tod;		//	CAT01
        };
    };
    float tod;				//	CAT01 CAT02 CAT08 CAT10
    float tod_stamp;			//	CAT01 CAT02 CAT08 CAT10
    unsigned long id;			// 	id
    unsigned long index;		//	index if available
};

struct bds30 {
    char str30[3*7+1];             //      printable bds3,0
    unsigned char ara41;                //      CAT48
    unsigned char ara42;                //      CAT48
    unsigned char ara43;                //      CAT48
    unsigned char ara44;                //      CAT48
    unsigned char ara45;                //      CAT48
    unsigned char ara46;                //      CAT48
    unsigned char ara47;                //      CAT48
    unsigned char tti;                  //      CAT48 (threat type indicator)
    union {
        struct {
            int tid_ms;                 //       (threat identity data mode s)
        };
        struct {
            int tid_modec;              //      (threat identity data ssr)
            float tid_rangef;
            int tid_bearing;
        };
    };
};