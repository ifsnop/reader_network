#!/bin/bash

echo `date +"%Y/%m/%d %H:%M:%S"` rrdtool update $1.rrd $2:$3:$4:$5:$6:$7 >> output.lst
rrdtool update $1.rrd $2:$3:$4:$5:$6:$7
 
