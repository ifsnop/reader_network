/********************************************/
/* crc32.h 0.0.0 (1999-Oct-17-Thu)          */
/* Adam M. Costello <amc@cs.berkeley.edu>   */
/********************************************/

/* Compute a 32-bit cyclic redundancy code (CRC) of a sequence of */
/* bytes, the same one used in Portable Network Graphics (PNG).   */
/* Note that this differs from the one used by the UNIX cksum     */
/* command, which uses the opposite bit ordering conventions, a   */
/* different initialization, and appends a length field to the    */
/* input byte stream.                                             */

/* This is ANSI C code. */


#ifndef CRC_H
#define CRC_H


#include <limits.h>
#include <stddef.h>


/* crc32_t is an unsigned integral type    */
/* with at least 32 bits (but maybe more): */

#if UINT_MAX >= 0xffffffff
typedef unsigned int crc32_t;
#else
typedef unsigned long crc32_t;
#endif

void setup_crc32_table(void);

crc32_t crc32(const unsigned char *bytes, size_t n);

    /* Returns the 32-bit CRC of the first n bytes in the array.  */
    /* Each byte must be in the range 0..255.  This is equivalent */
    /* to crc32_update(0xffffffff,bytes,n) ^ 0xffffffff.          */


crc32_t crc32_update(crc32_t crc, const unsigned char *bytes, size_t n);

    /* To compute the CRC incrementally, pass the first set     */
    /* of bytes to this function with crc = 0xffffffff.  Then   */
    /* pass each subsequent set of bytes with crc equal to the  */
    /* return value from the previous call.  After all bytes    */
    /* have been processed, the CRC of the entire sequence of   */
    /* bytes is r ^ 0xffffffff, where r is the return value     */
    /* from the last call, and ^ is the C bitwise xor operator, */
    /* not exponentiation.                                      */


#endif /* CRC_H */

