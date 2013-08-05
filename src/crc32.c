/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).
*/


#include "crc32.h"

crc32_t crc32_table[256];      /* CRCs of all 8-bit messages.    */
//static crc32_t crc32_table[256];      /* CRCs of all 8-bit messages.    */
//static int crc32_table_computed = 0;  /* Flag: Has table been computed? */


/* Make the table: */

//static 
void setup_crc32_table(void)
{
  crc32_t c;
  int i, k;

  for (i = 0;  i < 256;  ++i) {
    c = (crc32_t) i;

    for (k = 0;  k < 8;  ++k) {
      if (c & 1) c = 0xedb88320L ^ (c >> 1);
      else c >>= 1;
    }

    crc32_table[i] = c;
  }

//  crc32_table_computed = 1;
}


crc32_t crc32(const unsigned char *bytes, size_t n)
{
  return crc32_update(0xffffffff, bytes, n) ^ 0xffffffff;
}


crc32_t crc32_update(crc32_t crc, const unsigned char *bytes, size_t n)
{
  crc32_t c = crc;
  int i;

//  if (!crc32_table_computed) make_crc32_table();

  for (i = 0;  i < n;  i++) {
    c = crc32_table[(c ^ bytes[i]) & 0xff] ^ (c >> 8);
  }

  return c;
}
