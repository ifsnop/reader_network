#ifndef _CONFIG_SCAN_H
#define _CONFIG_SCAN_H

/*
 * Copyright (c) Abraham vd Merwe <abz@blio.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* multi-character tokens */
#define TOK_KEYWORD		0x1001
#define TOK_INTEGER		0x1002
#define TOK_STRING		0x1003
#define TOK_BOOLEAN		0x1004
#define TOK_ENUM		0x1005

typedef struct
{
   int fd;			/* file descriptor */
   int c;			/* cached character if not zero */
   union
	 {
		char *keyword;
		long int integer;
		char *string;
		int boolean;
	 } token;		/* token values */
   int line;		/* current line number */
} scan_t;

/*
 * NOTES:
 *
 * All of the functions below automatically print error messages to
 * the log system and return -1 if an error occurs. If successful,
 * they return 0 or in the case of scan() the token value
 */

/* initialize scanner */
extern int scan_open (scan_t *s,const char *filename);

/* scan for next token */
extern int scan (scan_t *s);

/* clean up */
extern void scan_close (scan_t *s);

#endif	/* #ifndef _CONFIG_SCAN_H */
