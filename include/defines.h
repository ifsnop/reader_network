/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2016 Diego Torres <diego dot torres at gmail dot com>

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


#define VERSION "0.70"
#define COPYRIGHT_NOTICE " v%s Copyright (C) 2002 - 2016 Diego Torres\n\n" \
    "This program comes with ABSOLUTELY NO WARRANTY.\n" \
    "This is free software, and you are welcome to redistribute it\n" \
    "under certain conditions; see COPYING file for details.\n\n"

#if defined(__sun)
#define ARCH "SOL"
#endif
#if defined(__linux)
#define ARCH "LNX"
#endif

#define TEXT_LENGTH_SHORT 4
#define TEXT_LENGTH_LONG  20

#define MULTICAST_PLOTS_GROUP "224.0.0.49"
#define MULTICAST_PLOTS_PORT 7001

#define UNICAST_PLOTS "172.88.2.221"
#define UNICAST_PLOTS_PORT 4001

#define MAX_PACKET_LENGTH 65532

#define MAX_RADAR_NUMBER 64
#define MAX_SEGMENT_NUMBER 3200

#define UPDATE_TIME 300.0
//#define UPDATE_TIME 10.0
#define UPDATE_TIME_RRD 300.0

#define SELECT_TIMEOUT 10

#define SCRM_MAX_QUEUE_SIZE 5000
#define SCRM_TIMEOUT 4

#define DEST_FILE_FORMAT_UNKNOWN 1
#define DEST_FILE_FORMAT_AST 2
#define DEST_FILE_FORMAT_GPS 4

