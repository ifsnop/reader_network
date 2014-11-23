/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2013 Diego Torres <diego dot torres at gmail dot com>

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

#ifdef CLIENT_RRD
void radar_delay_alloc(void);
void radar_delay_clear(void);
void update_RRD(int sac, int sic, int cat, int i, long timestamp, float cuenta, float max, 
    float min, float media, float stdev, float moda, float p99);
void update_calculations(struct datablock_plot * dbp);
#else
void update_calculations(struct datablock_plot * dbp);
#endif
