
struct datablock_plot {
    int sac; 				//	CAT01 CAT02 CAT08 CAT10
    int sic;				//	CAT01 CAT02 CAT08 CAT10
    int cat; 				// 	 // categoria original de la info
    int type; 				//	CAT01 CAT02 CAT08  CAT10 // psr-ssr-cmb-track/pasonorte.../sop-eop.../
    int plot_type; 			//	CAT10
    unsigned int track_plot_number; 	//	CAT01
    float rho;				//	CAT01
    float theta;			//	CAT01
    int modea;				//	CAT01
    int modec;				//	CAT01
    int modec_status;			//	CAT01
    int modea_status;			//	CAT01
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

