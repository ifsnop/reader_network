
# -*- sh -*-

#  Copyright (c) Abraham vd Merwe <abz@blio.com>
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the author nor the names of other contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
#  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# path to toplevel directory from here
TOPDIR = ..

# subdirectories (leave as is if there is no subdirectories)
DIR =

# names of object files
OBJ =

# program name (leave as is if there is no program)
PRG =

# library name (leave as is if there is no library)
LIB =

include $(TOPDIR)/paths.mk
include $(TOPDIR)/defs.mk
include $(TOPDIR)/vars.mk
include $(TOPDIR)/rules.mk

MAN = mem_open.3debug			\
	log_open.3debug				\
	hexdump.3debug

MEM_LNK = mem_check.3debug.gz	\
	mem_alloc.3debug.gz			\
	mem_realloc.3debug.gz		\
	mem_free.3debug.gz			\
	mem_close.3debug.gz

LOG_LNK = log_close.3debug.gz	\
	log_reset.3debug.gz			\
	log_flush.3debug.gz			\
	log_printf.3debug.gz		\
	log_vprintf.3debug.gz		\
	log_putc.3debug.gz			\
	log_puts.3debug.gz

MAN_LNK = $(MEM_LNK) $(LOG_LNK)

install::
	$(INSTALL) -d $(mandir)/man3
	set -e; for F in $(MAN); do $(INSTALL) -c -m 0644 $$F $(mandir)/man3 && gzip -9 $(mandir)/man3/$$F; done
	set -e; for F in $(MEM_LNK); do ln -sf mem_open.3debug.gz $(mandir)/man3/$$F; done
	set -e; for F in $(LOG_LNK); do ln -sf log_open.3debug.gz $(mandir)/man3/$$F; done

uninstall::
	set -e; for F in $(MAN_LNK) $(MAN); do rm -f $(mandir)/man3/$$F; done

