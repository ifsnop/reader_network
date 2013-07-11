
/* Based on the Memory Allocation Debugging system by Janne Kukonlehto */

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
#include <sys/types.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include <libdebug/memory.h>

#ifndef DEBUG_LOG
#include <libdebug/log.h>
#else	/* #ifndef DEBUG_LOG */
#define log_printf fprintf
#define LOG_ERROR stderr
#endif	/* #ifndef DEBUG_LOG */

#ifdef DEBUG_MEM
#define DPRINTF(fmt,args...) do {						\
		fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);	\
		fprintf(stderr,fmt,##args);						\
		fflush(stderr);									\
	} while (0)
#else	/* #ifdef DEBUG_MEM */
#define DPRINTF(fmt,args...)
#endif	/* #ifdef DEBUG_MEM */

/* signature for detecting overwrites */
#define MEM_SIGNATURE (('M' << 24) | ('L' << 16) | ('D' << 8) | 'S')

typedef struct mem_node
{
   uint32_t *head_sig;
   const char *file;
   int line;
   const char *function;
   void *data;
   uint32_t *tail_sig;
   size_t size;
   struct mem_node *next;
} mem_t;

static mem_t *mem_areas = NULL;
static void (*fail_stub)(const char *fmt,...);

static const char header[] =
  "Memory Leak Detection System\n"
  "----------------------------\n";

static void *xmalloc (const char *file,int line,const char *function,size_t size)
{
   void *ptr;

   if ((ptr = malloc (size)) == NULL)
	 fail_stub ("malloc(size: %u): %m\n"
				"Attempted allocation in file \"%s\" at line %d in function %s()\n",
				size,
				file,line,function);

   return (ptr);
}

#ifndef REALLOC_BUG_WORKAROUND
static void *xrealloc (const char *file,int line,const char *function,void *ptr,size_t size)
{
   void *ptr2;

   if ((ptr2 = realloc (ptr,size)) == NULL)
	 fail_stub ("realloc(ptr: %p, size: %u): %m\n"
				"Attempted allocation in file \"%s\" at line %d in function %s()\n",
				ptr,size,
				file,line,function);

   return (ptr2);
}
#endif	/* #ifndef REALLOC_BUG_WORKARAOUND */

static void mem_abort1 (const char *msg,mem_t *area,const char *file,int line,const char *function)
{
   fail_stub ("%s%s\n"
			  "Allocated in file \"%s\" at line %d in function %s()\n"
			  "Discovered in file \"%s\" at line %d in function %s()\n",
			  header,msg,
			  area->file,area->line,area->function,
			  file,line,function);
}

static void mem_abort2 (const char *msg,const char *file,int line,const char *function)
{
   fail_stub ("%s%s\n"
			  "Discovered in file \"%s\" at line %d in function %s()\n",
			  header,msg,
			  file,line,function);
}

/*
 * Check all the allocated memory areas. This is called every
 * time memory is allocated or freed. You can also call it
 * anytime you think memory might be corrupted.
 */
void mem_check_stub (const char *file,int line,const char *function)
{
   char str[100];
   mem_t *tmp;
//   static int round; // memory.c:134:15: warning: variable ‘round’ set but not used [-Wunused-but-set-variable]

//   round=0; // memory.c:136:15: warning: variable ‘round’ set but not used [-Wunused-but-set-variable]
   for (tmp = mem_areas; tmp != NULL; tmp = tmp->next)
	 {
		if (*(tmp->head_sig) != MEM_SIGNATURE)
		  {
			 sprintf (str,"Overwrite error: Bad head signature in area %p\n",tmp->data);
			 mem_abort1 (str,tmp,file,line,function);
		  }

		if (*(tmp->tail_sig) != MEM_SIGNATURE)
		  {
			 sprintf (str,"Overwrite error: Bad tail signature in area %p\n",tmp->data);
			 mem_abort1 (str,tmp,file,line,function);
		  }
	 }
}

/*
 * Allocate a memory area. Used instead of malloc()
 */
void *mem_alloc_stub (size_t size,const char *file,int line,const char *function)
{
   void *area;
   mem_t *tmp;
   size_t old_size = size;

   DPRINTF("called %s(%u)\n",__FUNCTION__,size);

   mem_check_stub (file,line,function);

   size = (size + 3) & (~3);	/* align on dword boundary */
   area = xmalloc (file,line,function,size + 2 * sizeof (uint32_t));

   tmp = (mem_t *) xmalloc (file,line,function,sizeof (mem_t));
   tmp->head_sig = (uint32_t *) area;
   tmp->data = area + sizeof (uint32_t);
   tmp->tail_sig = (uint32_t *) (area + size + sizeof (uint32_t));
   *(tmp->head_sig) = *(tmp->tail_sig) = MEM_SIGNATURE;
   tmp->file = file;
   tmp->line = line;
   tmp->function = function;
   tmp->size = old_size;
   tmp->next = mem_areas;
   mem_areas = tmp;

   return (tmp->data);
}

/*
 * Re-allocate a memory area. Used instead of realloc()
 */
void *mem_realloc_stub (void *ptr,size_t size,const char *file,int line,const char *function)
{
   mem_t *tmp;
   char str[100];
#ifndef REALLOC_BUG_WORKAROUND
   void *area;
#else	/* #ifndef REALLOC_BUG_WORKAROUND */
   void *ptr2;
#endif	/* #ifndef REALLOC_BUG_WORKAROUND */
   size_t old_size = size;

   DPRINTF("called %s(%p,%u)\n",__FUNCTION__,ptr,size);

   if (ptr == NULL) return (mem_alloc_stub (size,file,line,function));

   mem_check_stub (file,line,function);

   for (tmp = mem_areas; tmp != NULL && tmp->data != ptr; tmp = tmp->next) ;

   if (tmp == NULL)
	 {
		sprintf (str,"Attempted to realloc unallocated pointer: %p\n",ptr);
		mem_abort2 (str,file,line,function);
	 }

#ifndef REALLOC_BUG_WORKAROUND
   size = (size + 3) & (~3);	/* align on dword boundary */
   area = xrealloc (file,line,function,tmp->head_sig,size + 2 * sizeof (uint32_t));

   tmp->head_sig = (uint32_t *) area;
   tmp->data = area + sizeof (uint32_t);
   tmp->tail_sig = (uint32_t *) (area + size + sizeof (uint32_t));
   *(tmp->head_sig) = *(tmp->tail_sig) = MEM_SIGNATURE;
   tmp->file = file;
   tmp->line = line;
   tmp->function = function;
   tmp->size = old_size;

   return (tmp->data);
#else	/* #ifndef REALLOC_BUG_WORKAROUND */
   old_size = old_size > tmp->size ? tmp->size : old_size;

   if ((ptr2 = mem_alloc_stub (size,file,line,function)) == NULL)
	 return (NULL);

   memcpy (ptr2,ptr,old_size);
   mem_free_stub (ptr,file,line,function);
   ptr = ptr2;

   return (ptr2);
#endif	/* #ifndef REALLOC_BUG_WORKAROUND */
}

/*
 * Free a memory area. Used instead of free()
 */
void mem_free_stub (void *ptr,const char *file,int line,const char *function)
{
   char str[100];
   mem_t *prev,*cur;

   DPRINTF("called %s(%p)\n",__FUNCTION__,ptr);

   mem_check_stub (file,line,function);

   if (ptr == NULL)
	 mem_abort2 ("Attempted to free a NULL pointer\n",file,line,function);

   for (cur = mem_areas,prev = NULL; cur != NULL && cur->data != ptr; prev = cur, cur = cur->next) ;

   if (cur == NULL)
	 {
		sprintf (str,"Attempted to free an unallocated pointer: %p\n",ptr);
		mem_abort2 (str,file,line,function);
	 }

   free (cur->head_sig);

   /* now the hard part: deleting the node from the linked list (: */

   /* case 1: middle node */
   if (prev != NULL && cur->next != NULL)
	 prev->next = cur->next;

   /* case 2: root node */
   else if (prev == NULL)
	 mem_areas = cur->next;

   /* case 3: last node */
   else
	 prev->next = NULL;

   free (cur);
}

static void fail_default_stub (const char *fmt, ...)
{
   va_list ap;
   va_start (ap,fmt);
   if (fmt != NULL) log_vprintf (LOG_ERROR,fmt,ap);
   va_end (ap);
   exit (EXIT_FAILURE);
}

/*
 * Initialize the memory debugging system. You can specify
 * a function that should be called whenever something bad
 * happens, or NULL in which case the default error handler
 * will be used.
 */
void mem_open (void (*fail)(const char *fmt, ...))
{
   fail_stub = fail != NULL ? fail : fail_default_stub;
}

/*
 * Clean up and print a list of unfreed memory areas
 */
void mem_close (void)
{
   mem_t *tmp;

   mem_check ();

   if (mem_areas != NULL)
	 {
		log_printf (LOG_ERROR,header);

		while (mem_areas != NULL)
		  {
			 log_printf (LOG_ERROR,
						 "Unfreed pointer: %p\n"
						 "   Allocated in file \"%s\" at line %d in function %s()\n",
						 mem_areas->data,
						 mem_areas->file,mem_areas->line,mem_areas->function);
			 tmp = mem_areas->next;
			 free (mem_areas);
			 mem_areas = tmp;
		  }

		fail_stub (NULL);
	 }
}

