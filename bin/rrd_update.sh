#!/bin/bash

#sprintf(tmp, "rrd_update.sh %03d_%03d_%03d %ld %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f", 
#	sac, sic, cat, timestamp, cuenta, max, min, media, stdev, p99);
#	$1             $2         $3      $4   $5   $6     $7     $8

if [[ -z $REGION ]]; then
    exit
fi

REGION=${REGION:-"NA"}

#echo `date +"%Y/%m/%d %H:%M:%S"` "REPLACE INTO availability2 (sac_sic_cat,region,timestamp,cuenta,max,min,media,stdev,p99,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,$4,$5,$6,$7,$8,now());" >> /tmp/t.t
#exit
            
echo `date +"%Y/%m/%d %H:%M:%S"` rrdtool update /var/www/lighttpd/cocirv3/rrds/${REGION}_${1}.rrd $2:$4:$5:$6:$7:$8 >> /home/eval/cocir/logs/${REGION}_${1}.log
rrdtool update /var/www/lighttpd/cocirv3/rrds/${REGION}_${1}.rrd $2:$4:$5:$6:$7:$8
rrdtool update /var/www/lighttpd/cocirv4/rrds/${REGION}_${1}.rrd $2:$4:$5:$6:$7:$8

echo `date +"%Y/%m/%d %H:%M:%S"` "REPLACE INTO availability2 (sac_sic_cat,region,timestamp,cuenta,max,min,media,stdev,p99,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,$4,$5,$6,$7,$8,now());" >> /home/eval/cocir/logs/${REGION}_${1}.log 2>&1
mysql -u root cocir -e "REPLACE INTO availability2 (sac_sic_cat,region,timestamp,cuenta,max,min,media,stdev,p99,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,$4,$5,$6,$7,$8,now());" >> /home/eval/cocir/logs/${REGION}_${1}.log 2>&1
#echo `date +"%Y/%m/%d %H:%M:%S"` "REPLACE INTO availability (sac_sic_cat,region,timestamp,cuenta,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,now());" >> /home/eval/cocir/logs/${REGION}_${1}.sql
#mysql -u root cocir -e "REPLACE INTO availability (sac_sic_cat,region,timestamp,cuenta,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,now());" >> /home/eval/cocir/logs/${REGION}_${1}.log 2>&1
