
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

#DEBUG = "yes"

#CROSS = arm-linux-

# version
MAJOR = $(shell pwd | sed 's|^.*/libconfig32-\(.*\)$$|\1|g' | cut -f1 -d.)
MINOR = $(shell pwd | sed 's|^.*/libconfig32-\(.*\)$$|\1|g' | cut -f2 -d.)
PATCH = $(shell pwd | sed 's|^.*/libconfig32-\(.*\)$$|\1|g' | cut -f3 -d.)

ifndef prefix
prefix = $(DESTDIR)/usr/local
endif

libdir = $(prefix)/lib
includedir = $(prefix)/include

       ########### NOTHING TO EDIT BELOW THIS ###########

CC = $(CROSS)colorgcc

ifeq ($(shell which $(CC)),)
CC = $(CROSS)gcc
endif

ifeq ($(shell which $(CC)),)
CC = gcc
endif

INSTALL = install -v

CPPFLAGS = -m32 -I$(includedir) -I.
CFLAGS = -m32 -Wall -Winline -fPIC -Os -pipe
LDFLAGS = -m32 -L$(libdir) -L. -Wl,-rpath,.
LDLIBS = -m32 -lconfig32 -ldebug32

ifeq ($(DEBUG),"yes")
CFLAGS += -ggdb -g3
LDFLAGS += -ggdb -g3
endif

LD = ld

AR = ar
ARFLAGS = crv

RM = rm -f

LN = ln

OBJ = scan.o parse.o config.o
LIB = libconfig32
PRG = test_parse test_config

SRC = $(OBJ:%.o=%.c)

all: do-it-all

ifeq (.depends,$(wildcard .depends))
include .depends
do-it-all: with-depends
else
do-it-all: without-depends
endif

without-depends: depend with-depends

depend:
	$(RM) .depends
	set -e; for F in $(SRC); do $(CC) -MM $(CFLAGS) $(CPPFLAGS) $$F >> .depends; done

with-depends: $(LIB).a $(LIB).so.$(MAJOR).$(MINOR).$(PATCH) $(PRG)

test_parse: $(LIB).so main_parse.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
	
test_config: $(LIB).so main_config.o
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(LIB).a: $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

$(LIB).so.$(MAJOR).$(MINOR).$(PATCH): $(OBJ)
	$(CC) -shared -Wl,-soname -Wl,$(LIB).so.$(MAJOR) $^ -o $@
	ln -sf $@ $(LIB).so.$(MAJOR)
	ln -sf $(LIB).so.$(MAJOR) $(LIB).so

install: all
	$(INSTALL) -d $(libdir)
	$(INSTALL) -d $(includedir)/config
	$(INSTALL) -c -m 0755 $(LIB).so.$(MAJOR).$(MINOR).$(PATCH) $(libdir)
	$(INSTALL) -c -m 0644 $(LIB).a $(libdir)
	ln -sf $(LIB).so.$(MAJOR).$(MINOR).$(PATCH) $(libdir)/$(LIB).so.$(MAJOR)
	ln -sf $(LIB).so.$(MAJOR) $(libdir)/$(LIB).so
	$(INSTALL) -c -m 0644 *.h $(includedir)/config

uninstall:
	rm -rf $(DESTDIR)/$(includedir)/config
	rm -f $(DESTDIR)/$(libdir)/$(LIB).*

debian:
	dpkg-buildpackage -rfakeroot -k2B555AEE

clean:
	$(RM) .depends *~ $(OBJ) $(LIB).* $(PRG) *.o

distclean: clean
	rm -f {configure,build}-stamp
	rm -rf debian/$(PRG)
	rm -f debian/*.{debhelper,substvars} debian/files debian/*~
	rm -rf debian/{libconfig0,libconfig0-dev,tmp}

.PHONY: all clean distclean do-it-all depend with-depends without-depends install uninstall debian

