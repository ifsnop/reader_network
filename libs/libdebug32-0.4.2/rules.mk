
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

.PHONY:: all do-it-all dist depend with-depends without-depends clean distclean install uninstall

ifneq ($(LIB),)
ifneq ($(MAJOR),)
LIB_MAJOR := $(shell echo $(LIB) | sed 's|^\(lib.*\.so\.[0-9]\+\).*$$|\1|g')
LIB_SO := $(shell echo $(LIB_MAJOR) | sed 's|^\(lib.*\.so\).*$$|\1|g')
LIB_A := $(shell echo $(LIB_SO) | sed 's|^\(lib.*\)\.so$$|\1.a|g')
endif	# ifneq ($(MAJOR),)
endif	# ifneq ($(LIB),)

ANY = $(PRG) $(LIB) $(LIB_MAJOR) $(LIB_SO) $(LIB_A) $(OBJ)
SRC = $(OBJ:%.o=%.c)

all:: do-it-all

ifneq ($(DIR),)
all::
	for F in $(DIR); do $(MAKE) -C $$F all; done
endif	# ifneq ($(DIR),)

do-it-all::

ifneq ($(SRC),)
ifeq (.depends,$(wildcard .depends))
include .depends
do-it-all:: with-depends
else	# ifeq (.depends,$(wildcard .depends))
do-it-all:: without-depends
endif	# ifeq (.depends,$(wildcard .depends))
endif	# ifneq ($(SRC),)

# we have to call make here again, otherwise it doesn't know about
# files created by the dist rule
without-depends: dist depend
	$(MAKE) with-depends

depend:
	rm -f .depends
	set -e; for F in $(SRC); do $(CC) -MM $(CFLAGS) $(CPPFLAGS) $$F >> .depends; done

with-depends: $(ANY)

$(PRG):: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

ifeq ($(_STRIP),"yes")
$(PRG):: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
	$(STRIP) $(STRIPFLAGS) $@
endif	# ifeq ($(_STRIP),"yes")

clean::
	rm -f .depends *~ $(ANY) gmon.out a.out core

distclean:: clean

install:: all

uninstall::

ifneq ($(DIR),)
distclean::
	for F in $(DIR); do $(MAKE) -C $$F distclean; done

install::
	for F in $(DIR); do $(MAKE) -C $$F install; done

uninstall::
	for F in $(DIR); do $(MAKE) -C $$F uninstall; done
endif	# ifneq ($(DIR),)

ifeq ($(suffix $(LIB)),.a)
$(LIB):: $(OBJ)
	$(AR) $(ARFLAGS) $@ $^
else	# ifeq ($(suffix $(LIB)),.a)
ifneq ($(MAJOR),)
$(LIB_A): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB_A) $^

$(LIB):: $(LIB_A)

$(LIB):: $(OBJ)
	$(CC) $(LDFLAGS) -shared -Wl,-soname -Wl,$(LIB_MAJOR) $^ -o $@ $(LDLIBS)
	ln -sf $@ $(LIB_MAJOR)
	ln -sf $(LIB_MAJOR) $(LIB_SO)
else	# ifneq ($(MAJOR),)
$(LIB):: $(OBJ)
	$(CC) $(LDFLAGS) -shared $^ -o $@ $(LDLIBS)
endif	# ifneq ($(MAJOR),)
ifeq ($(_STRIP),"yes")
$(LIB):: $(OBJ)
	$(STRIP) $(STRIPFLAGS) $@
endif	# ifeq ($(_STRIP),"yes")
endif	# ifeq ($(suffix $(LIB)),.a)

