/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2014 Diego Torres <diego dot torres at gmail dot com>

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

#define T_OK 0
#define T_ERROR -1
#define T_YES 1
#define T_NO 0

#define CAT_01 1
#define CAT_02 2
#define CAT_08 8
#define CAT_10 10
#define CAT_19 19
#define CAT_20 20
#define CAT_21 21
#define CAT_34 34
#define CAT_48 48
#define CAT_255 255

#define NO_DETECTION 0
#define TYPE_C1_PSR 1
#define TYPE_C1_SSR 2
#define TYPE_C1_CMB 4
#define TYPE_C1_FIXED_TRANSPONDER 8

#define TYPE_C48_PSR 1
#define TYPE_C48_SSR 2
#define TYPE_C48_CMB 4
#define TYPE_C48_SSRSGEN 8
#define TYPE_C48_SSRSROL 16
#define TYPE_C48_CMBSGEN 32
#define TYPE_C48_CMBSROL 64
#define TYPE_C48_FIXED_TRANSPONDER 128

#define IS_ERROR 0
#define IS_SACSIC 1
#define IS_TYPE 2
#define IS_MEASURED_POLAR 4
#define IS_MEASURED_CARTE 8
#define IS_MODEA 16
#define IS_MODEC 32
#define IS_MODES 2048
#define IS_TRUNCATED_TOD 64
#define IS_RADAR_RESPONSES 128
#define IS_TRACK 256
#define IS_TOD 512
#define IS_PLOT 1024
#define IS_MODES_ADDRESS 2048
#define IS_AIRCRAFT_ID 4096
#define IS_COMM_CAP 8192

#define BDS_EMPTY 0
#define BDS_10 1
#define BDS_17 2
#define BDS_30 4
#define BDS_40 8
#define BDS_50 16
#define BDS_60 32

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

#define TYPE_C10_TARGET_REPORT 1
#define TYPE_C10_START_UPDATE_CYCLE 2
#define TYPE_C10_PERIODIC_STATUS 4
#define TYPE_C10_EVENT_STATUS 8

#define TYPE_C10_PLOT_SSR_MULTI 1
#define TYPE_C10_PLOT_SSRS_MULTI 2
#define TYPE_C10_PLOT_ADSB 4
#define TYPE_C10_PLOT_PSR 8
#define TYPE_C10_PLOT_MAGNETIC 16
#define TYPE_C10_PLOT_HF_MULTI 32
#define TYPE_C10_PLOT_NOT_DEFINED 64
#define TYPE_C10_PLOT_OTHER 164

#define TYPE_C19_START_UPDATE_CYCLE 1
#define TYPE_C19_PERIODIC_STATUS 2
#define TYPE_C19_EVENT_STATUS 4

#define TYPE_C20_NONMODESMLAT 1
#define TYPE_C20_MODESMLAT 2
#define TYPE_C20_HFMLAT 4
#define TYPE_C20_VDLMLAT 8
#define TYPE_C20_UATMLAT 16
#define TYPE_C20_DMEMLAT 32
#define TYPE_C20_OTHERMLAT 64

#define TYPE_C20_RAB 128
#define TYPE_C20_SPI 256
#define TYPE_C20_CHN 512
#define TYPE_C20_GBS 1024
#define TYPE_C20_CRT 2048
#define TYPE_C20_SIM 4096
#define TYPE_C20_TST 8192

#define TYPE_C34_NORTH_MARKER 1
#define TYPE_C34_SECTOR_CROSSING 2
#define TYPE_C34_GEOGRAPHICAL_FILTERING 4
#define TYPE_C34_JAMMING_STROBE 8

#define FILTER_NONE 0
#define FILTER_GROUND 1

// unsigned char full_tod[MAX_RADAR_NUMBER*TTOD_WIDTH]; /* 2 sacsic, 1 null, 3 full_tod, 2 max_ttod */

typedef struct {
    ssize_t size_datablock;
    unsigned char *ptr_raw;
    int filter_type;
} filter_struct;

void ttod_put_full(unsigned char sac, unsigned char sic, unsigned char *ptr_full_tod);
float ttod_get_full(int sac, int sic, unsigned char *ptr_ttod, unsigned long id);
int ast_procesarCAT01(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT02(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT08(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT10(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT19(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT20(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT21(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT34(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
int ast_procesarCAT48F(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar, filter_struct *fs);
int ast_procesarCAT62(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, bool enviar);
void ast_output_datablock(unsigned char *ptr_raw, ssize_t size_datablock, unsigned long id, unsigned long index);
int ast_get_size_FSPEC(unsigned char *ptr_raw, ssize_t size_datablock);
inline char* parse_hora(float segs);
bool filter_test(unsigned char *ptr_raw, int ptr, int filter_type);
void decode_bds30(unsigned char * ptr_raw, int j, struct datablock_plot * dbp, struct bds30 * bds, char *stmt);

//void update_calculations(struct datablock_plot dbp);
