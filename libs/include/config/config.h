#ifndef _CONFIG_CONFIG_H
#define _CONFIG_CONFIG_H

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

#include <config/typedefs.h>

/* open configuration file */
extern bool cfg_open (const char *filename);

/* extract boolean value from configuration file */
extern bool cfg_get_bool (bool *value,const char *name);

/* extract integer value from configuration file */
extern bool cfg_get_int (long int *value,const char *name);

/* extract string from configuration file */
extern bool cfg_get_str (char **str,const char *name);

/* extract enumeration from configuration file */
extern bool cfg_get_enum (char **str,const char *name);

/* extract an array of integers from configuration file */
extern bool cfg_get_int_array (long int **array,int *n,const char *name);

/* extract an array of strings from configuration file */
extern bool cfg_get_str_array (char ***array,int *n,const char *name);

/* extract an array of enumerations from configuration file */
extern bool cfg_get_enum_array (char ***array,int *n,const char *name);

/* extract an array of boolean values from configuration file */
extern bool cfg_get_bool_array (bool **array,int *n,const char *name);

/* close configuration file */
extern void cfg_close (void);

/* usually the variable name is the same as the keyword string, so we
 * define some shortcuts for those cases */
#define cfg_get_boolean(x) cfg_get_bool(&x,#x)
#define cfg_get_integer(x) cfg_get_int(&x,#x)
#define cfg_get_string(x) cfg_get_str(&x,#x)
#define cfg_get_enumeration(x) cfg_get_enum(&x,#x)
#define cfg_get_boolean_array(x,n) cfg_get_bool_array(&x,n,#x)
#define cfg_get_integer_array(x,n) cfg_get_int_array(&x,n,#x)
#define cfg_get_string_array(x,n) cfg_get_str_array(&x,n,#x)
#define cfg_get_enumeration_array(x,n) cfg_get_enum_array(&x,n,#x)

#endif	/* #ifndef _CONFIG_CONFIG_H */
