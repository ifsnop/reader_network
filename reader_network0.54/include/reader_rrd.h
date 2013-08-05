
void radar_delay_alloc(void);
void radar_delay_clear(void);
void update_calculations(struct datablock_plot dbp);
void update_RRD(int sac, int sic, int cat, int i, long timestamp, float cuenta, float max, 
    float min, float media, float stdev, float moda, float p99);
