
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

#CROSS = arm-linux-

CFLAGS += -Wall -Wno-trigraphs -Os -pipe -fno-strict-aliasing -fno-common
CPPFLAGS += -I$(includedir) -I$(TOPDIR)/include
LDFLAGS += -L$(libdir)

STRIP = $(CROSS)strip
STRIPFLAGS = --strip-all --remove-section=.note --remove-section=.comment

ifeq ($(RESOLVE),"yes")
CPPFLAGS += -DGETHOSTBYNAME -DGETSERVBYNAME
endif	# ifeq ($(RESOLVE),"yes")

_STRIP = "yes"

ifeq ($(DEBUG),"yes")
_STRIP = "no"
CPPFLAGS += -DDEBUG
endif	# ifeq ($(DEBUG),"yes")

ifeq ($(PROFILE),"yes")
_STRIP = "no"
endif	# ifeq ($(PROFILE),"yes")

ifndef CROSS
COLOR = color
else	# ifndef CROSS
CFLAGS += -mapcs-32 -march=armv4 -mtune=strongarm1100 -mshort-load-bytes
endif	# ifndef CROSS

CC = $(CROSS)$(COLOR)gcc

ifeq ($(shell which $(CC)),)
CC = $(CROSS)gcc
endif	# ifeq ($(shell which $(CC)),)

ifeq ($(shell which $(CC)),)
CC = gcc
endif	# ifeq ($(shell which $(CC)),)

ifeq ($(DEBUG),"yes")
CFLAGS += -ggdb
LDFLAGS += -ggdb
endif	# ifeq ($(DEBUG),"yes")

ifeq ($(STRIP),"yes")
LDFLAGS += -s
endif	# ifeq ($(STRIP),"yes")

ifeq ($(PROFILE),"yes")
CFLAGS += -pg
endif	# ifeq ($(PROFILE),"yes")

ifneq ($(shell echo $(LIB) | grep -e '\.so'),)
CFLAGS += -fPIC
endif	# ifeq ($(suffix $(LIB)),.so)

INSTALL = install

AR = $(CROSS)ar
ARFLAGS = crv

