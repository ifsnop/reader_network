#ifndef _CONFIG_TYPEDEFS_H
#define _CONFIG_TYPEDEFS_H

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

#include <stdlib.h>

/*
 * Boolean definitions
 */

#ifndef bool
typedef int bool;
#endif	/* #ifndef bool */

#if !defined(false) || (false != 0)
#define false	(0)
#endif	/* #if !defined(false) || (false != (0)) */

#if !defined(true) || (true == false)
#define true	(!false)
#endif	/* #if !defined(true) || (true != (0)) */

/*
 * Exit status values
 */

#if !defined(EXIT_SUCCESS) || (EXIT_SUCCESS != 0)
#define EXIT_SUCCESS	(0)
#endif	/* #if !defined(EXIT_SUCCESS) || (EXIT_SUCCESS != (0)) */

#if !defined(EXIT_FAILURE) || (EXIT_FAILURE == EXIT_SUCCESS)
#define EXIT_FAILURE	(1)
#endif	/* #if !defined(EXIT_FAILURE) || (EXIT_FAILURE == EXIT_SUCCESS) */

#endif	/* #ifndef _CONFIG_TYPEDEFS_H */
