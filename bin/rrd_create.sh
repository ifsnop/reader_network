#!/bin/bash

#sprintf(tmp, "rrd_update.sh %03d_%03d_%03d %ld %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f", sac, sic, cat, timestamp, cuenta, max, min, media, stdev, p99);

if [[ -z $REGION ]]; then
    exit
fi

REGION=${REGION:-"NA"}

if [ -f /var/www/lighttpd/cocirv3/rrds/${REGION}_$1.rrd ]; then
    exit
fi

echo `date +"%Y/%m/%d %H:%M:%S"` rrdtool create /var/www/lighttpd/cocirv3/rrds/${REGION}_$1.rrd \
--step 300 \
--start $2 \
DS:max_ds:GAUGE:600:-16:16 \
DS:min_ds:GAUGE:600:-16:16 \
DS:media_ds:GAUGE:600:-16:16 \
DS:stdev_ds:GAUGE:600:-16:16 \
DS:p99_ds:GAUGE:600:-16:16 \
RRA:AVERAGE:0.5:1:600 \
RRA:AVERAGE:0.5:6:700 \
RRA:AVERAGE:0.5:24:775 \
RRA:AVERAGE:0.5:288:797 \
RRA:MAX:0.5:1:600 \
RRA:MAX:0.5:6:700 \
RRA:MAX:0.5:24:775 \
RRA:MAX:0.5:288:797 \
RRA:MIN:0.5:1:600 \
RRA:MIN:0.5:6:700 \
RRA:MIN:0.5:24:775 \
RRA:MIN:0.5:288:797 >> /home/eval/cocir/logs/${REGION}_$1.log

rrdtool create /var/www/lighttpd/cocirv3/rrds/${REGION}_$1.rrd \
--step 300 \
--start $2 \
DS:max_ds:GAUGE:600:-16:16 \
DS:min_ds:GAUGE:600:-16:16 \
DS:media_ds:GAUGE:600:-16:16 \
DS:stdev_ds:GAUGE:600:-16:16 \
DS:p99_ds:GAUGE:600:-16:16 \
RRA:AVERAGE:0.5:1:600 \
RRA:AVERAGE:0.5:6:700 \
RRA:AVERAGE:0.5:24:775 \
RRA:AVERAGE:0.5:288:797 \
RRA:MAX:0.5:1:600 \
RRA:MAX:0.5:6:700 \
RRA:MAX:0.5:24:775 \
RRA:MAX:0.5:288:797 \
RRA:MIN:0.5:1:600 \
RRA:MIN:0.5:6:700 \
RRA:MIN:0.5:24:775 \
RRA:MIN:0.5:288:797
