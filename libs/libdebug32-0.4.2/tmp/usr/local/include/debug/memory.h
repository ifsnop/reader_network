#ifndef _DEBUG_MEMORY_H
#define _DEBUG_MEMORY_H

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

#include <sys/cdefs.h>
#include <sys/types.h>

/*
 * Initialize the memory debugging system. You can specify
 * a function that should be called whenever something bad
 * happens, or NULL in which case the default error handler
 * will be used.
 */
extern void mem_open (void (*fail)(const char *fmt,...));

/*
 * Check all the allocated memory areas. This is called every
 * time memory is allocated or freed. You can also call it
 * anytime you think memory might be corrupted.
 */
#define mem_check() mem_check_stub(__FILE__,__LINE__,__FUNCTION__)
extern void mem_check_stub (const char *file,int line,const char *function);

/*
 * Allocate a memory area. Used instead of malloc()
 */
#define mem_alloc(size) mem_alloc_stub(size,__FILE__,__LINE__,__FUNCTION__)
extern void *mem_alloc_stub (size_t size,const char *file,int line,const char *function)
  __attribute_malloc__;

/*
 * Re-allocate a memory area. Used instead of realloc()
 */
#define mem_realloc(ptr,size) mem_realloc_stub(ptr,size,__FILE__,__LINE__,__FUNCTION__)
extern void *mem_realloc_stub (void *ptr,size_t size,const char *file,int line,const char *function)
  __attribute_malloc__;

/*
 * Free a memory area. Used instead of free()
 */
#define mem_free(ptr) mem_free_stub(ptr,__FILE__,__LINE__,__FUNCTION__)
extern void mem_free_stub (void *ptr,const char *file,int line,const char *function);

/*
 * Clean up and print a list of unfreed memory areas
 */
extern void mem_close (void);

#endif	/* #ifndef _DEBUG_MEMORY_H */
