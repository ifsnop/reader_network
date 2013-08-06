#define T_OK 0
#define T_ERROR -1

#define CAT_01 1
#define CAT_02 2
#define CAT_08 8
#define CAT_10 10
#define CAT_255 255

#define NO_DETECTION 0
#define TYPE_C1_PSR 1
#define TYPE_C1_SSR 2
#define TYPE_C1_CMB 4
#define FROM_C1_FIXED_TRANSPONDER 8

#define IS_ERROR 0
#define IS_SACSIC 1
#define IS_TYPE 2
#define IS_MEASURED_POLAR 4
#define IS_MEASURED_CARTE 8
#define IS_MODEA 16
#define IS_MODEC 32
#define IS_TRUNCATED_TOD 64
#define IS_RADAR_RESPONSES 128
#define IS_TRACK 256
#define IS_TOD 512

#define STATUS_MODEC_GARBLED 1
#define STATUS_MODEC_NOTVALIDATED 2

#define STATUS_MODEA_GARBLED 1
#define STATUS_MODEA_NOTVALIDATED 2
#define STATUS_MODEA_SMOOTHED 4

#define TTOD_WIDTH 8

#define TYPE_C2_NORTH_MARKER 1
#define TYPE_C2_SECTOR_CROSSING 2
#define TYPE_C2_SOUTH_MARKER 4
#define TYPE_C2_START_BLIND_ZONE_FILTERING 8
#define TYPE_C2_STOP_BLIND_ZONE_FILTERING 16

#define TYPE_C8_CONTROL_SOP 1
#define TYPE_C8_CONTROL_EOP 2

unsigned char full_tod[MAX_RADAR_NUMBER*TTOD_WIDTH]; /* 2 sacsic, 1 null, 3 full_tod, 2 max_ttod */

void ttod_put_full(unsigned char sac, unsigned char sic, unsigned char *ptr_full_tod);
float ttod_get_full(int sac, int sic, unsigned char *ptr_ttod);
int ast_procesarCAT01(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id);
int ast_procesarCAT02(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id);
int ast_procesarCAT08(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id);
void ast_output_datablock(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, unsigned long index);
int ast_get_size_FSPEC(unsigned char *ptr_raw, ssize_t size_datablock);
inline char* parse_hora(float segs);

