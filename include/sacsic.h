
#define GET_SAC_SHORT 1
#define GET_SAC_LONG  2
#define GET_SIC_SHORT 4
#define GET_SIC_LONG  8


char *ast_get_SACSIC(unsigned char *sac, unsigned char *sic, int action);
//char *ast_get_SACSIC(int sac, int sic, int action);
