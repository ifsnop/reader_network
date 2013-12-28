/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2014 Diego Torres <diego dot torres at gmail dot com>

This file is part of the reader_network utils.

reader_network is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

reader_network is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with reader_network. If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <values.h>
#include <mntent.h>
#include <libgen.h>
#include <sys/statvfs.h>
#include "libdebug/memory.h"
#include "libdebug/log.h"
#include "libdebug/hex.h"
#include "libconfig/config.h"
#include "startup.h"
#include "defines.h"
#include "asterix.h"
#include "sacsic.h"
#include "dbp.h"
#include "helpers.h"
#include "crc32.h"
#include "red_black_tree.h"
#include "reader_rrd.h"
#include "curl/multi.h"
#include "md5.h"
