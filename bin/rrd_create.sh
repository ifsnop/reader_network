#!/bin/bash
exit
rm $1.rrd 2> /dev/null

echo rrdtool create $1.rrd \
--step 300 \
--start $2 \
DS:max_ds:GAUGE:600:-16:16 \
DS:min_ds:GAUGE:600:-16:16 \
DS:media_ds:GAUGE:600:-16:16 \
DS:moda_ds:GAUGE:600:-16:16 \
DS:desv_ds:GAUGE:600:-16:16 \
RRA:AVERAGE:0.5:1:600 \
RRA:AVERAGE:0.5:6:700 \
RRA:AVERAGE:0.5:24:775 \
RRA:AVERAGE:0.5:200:797 \
RRA:MAX:0.5:1:600 \
RRA:MAX:0.5:6:700 \
RRA:MAX:0.5:24:775 \
RRA:MAX:0.5:288:797 \
RRA:MIN:0.5:1:600 \
RRA:MIN:0.5:6:700 \
RRA:MIN:0.5:24:775 \
RRA:MIN:0.5:288:797 >> output.lst

rrdtool create $1.rrd \
--step 300 \
--start $2 \
DS:max_ds:GAUGE:600:-16:16 \
DS:min_ds:GAUGE:600:-16:16 \
DS:media_ds:GAUGE:600:-16:16 \
DS:moda_ds:GAUGE:600:-16:16 \
DS:desv_ds:GAUGE:600:-16:16 \
RRA:AVERAGE:0.5:1:600 \
RRA:AVERAGE:0.5:6:700 \
RRA:AVERAGE:0.5:24:775 \
RRA:AVERAGE:0.5:200:797 \
RRA:MAX:0.5:1:600 \
RRA:MAX:0.5:6:700 \
RRA:MAX:0.5:24:775 \
RRA:MAX:0.5:288:797 \
RRA:MIN:0.5:1:600 \
RRA:MIN:0.5:6:700 \
RRA:MIN:0.5:24:775 \
RRA:MIN:0.5:288:797
