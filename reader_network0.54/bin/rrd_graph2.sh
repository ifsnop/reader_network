#!/bin/bash
# --height=150 --width=500

rrdtool graph $1_cat1_day.png \
--imgformat=PNG \
--title="Radar $1 CAT 1" \
--start=-86400 --end=-300 \
--base=1000 \
--height=300 \
--width=750 \
--alt-autoscale \
--units-exponent -3 \
--vertical-label="milisegundos" \
--upper-limit 1 \
--lower-limit 0 \
--rigid \
DEF:media_1=$1_CAT1.rrd:media_ds:AVERAGE LINE1:media_1#000099:"media cat1" \
DEF:media_2=$1_CAT2.rrd:media_ds:AVERAGE LINE1:media_2#009900:"media cat1" \
DEF:min_1=$1_CAT1.rrd:min_ds:AVERAGE LINE1:min_1#0000FF:"min cat1" \
DEF:max_1=$1_CAT1.rrd:max_ds:AVERAGE LINE1:max_1#0000FF:"max cat1" \
#DEF:desv_1=$1_CAT1.rrd:desv_ds:AVERAGE \
#CDEF:sigma3_1_sup=media_1,desv_1,3,*,+ \
#CDEF:sigma3_1_inf=media_1,desv_1,3,*,- \
#LINE1:sigma3_1_sup#FF0000:"desv sup cat1" \
#LINE1:sigma3_1_inf#FF0000:"desv inf cat1" 
#DEF:media_2=ESPBarcelona_CAT2.rrd:media_ds:AVERAGE LINE2:media_2#009900:"media cat2" \
#DEF:min_2=ESPBarcelona_CAT2.rrd:min_ds:AVERAGE LINE1:min_2#00FF00:"min cat2" \
#DEF:max_2=ESPBarcelona_CAT2.rrd:max_ds:AVERAGE LINE1:max_2#00FF00:"max cat2"

rrdtool graph $1_cat2_day.png \
--imgformat=PNG \
--title="Radar $1 CAT 2" \
--start=-86400 --end=-300 \
--base=1000 \
--height=300 \
--width=750 \
--alt-autoscale \
--units-exponent -3 \
--vertical-label="segundos" \
DEF:media_2=$1_CAT2.rrd:media_ds:AVERAGE LINE2:media_2#000099:"media cat1" \
DEF:min_2=$1_CAT2.rrd:min_ds:AVERAGE LINE1:min_2#0000FF:"min cat1" \
DEF:max_2=$1_CAT2.rrd:max_ds:AVERAGE LINE1:max_2#0000FF:"max cat1" 
#CDEF:max_calc=media_2,media_2,0.50,*,- \
#CDEF:min_calc=media_2,media_2,0.50,*,+ \
#LINE2:max_calc#FF0000:"max calc" \
#LINE2:min_calc#FF0000:"min calc" 

#DEF:desv_2=$1_CAT2.rrd:desv_ds:AVERAGE \
#--upper-limit 1 \
#--lower-limit 0 \
#--rigid \
#GPRINT:media:LAST:"Actual\:%8.2lf %s" \
#GPRINT:media:AVERAGE:"Media\:%8.2lf %s\n"

#--start 1078614000 --end 1078685904 \
#--start=-86400 --end=-300 \
