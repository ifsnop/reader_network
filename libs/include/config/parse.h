#ifndef _CONFIG_PARSE_H
#define _CONFIG_PARSE_H

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

#include "scan.h"

/*
 * A quick word about the parser data structure
 * --------------------------------------------
 *
 * Configuration files is made up of statements and sections. Each section have
 * it's own scope and must contain at least one statement / section.
 *
 * When parse() is called, the entire contents of the specified configuration
 * file is dumped into the data structure below (section_t). secion_t is a tree
 * of which each node contains ``n'' children. The statements in that section
 * is stored in the stmt_t structure which is a circular linked list. The first
 * statement will be at the head of the ring (i.e. if you traverse the ring in
 * the normal order (s, s->next, s->next->next, etc.) you will retrieve the
 * statements in the same order as in the configuration file.
 *
 * There can be four types of data: boolean values (TOK_BOOLEAN), integers
 * (TOK_INTEGER), strings (TOK_STRING), and enumerations (TOK_ENUM), which are
 * identical to strings. These data types are stored in the arg_t structure.
 *
 * For more information, see main_parse.c (the test example) and CAVEATS.
 */

typedef union
{
   char *string;
   long int integer;
   int boolean;
} arg_t;

typedef struct stmt_r
{
   char *name;					/* variable name */
   int type;					/* argument type (see TOK_* in scan.h) */
   arg_t *args;					/* arguments */
   int n;						/* number of arguments */
   struct stmt_r *prev;
   struct stmt_r *next;
} stmt_t;

typedef struct section_r
{
   char *name;					/* section name */
   stmt_t *stmt;				/* list of statements in this section */
   struct section_r **child;
   int n;
} section_t;

/*
 * parse filename. return a linked list containing all the parsed
 * statements if successful, NULL if some error occurred. the
 * function automatically logs all errors
 */
extern section_t *parse (const char *filename);

/*
 * free all memory occupied by the specified parse structure
 */
extern void parse_destroy (section_t **sections);

#endif	/* #ifndef _CONFIG_PARSE_H */
