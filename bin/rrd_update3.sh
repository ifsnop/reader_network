#!/bin/bash

#sprintf(tmp, "rrd_update.sh %03d_%03d_%03d %s %ld %3.3f %3.3f %3.3f %3.3f %3.3f %3.3f", 
#	sac, sic, cat, region, timestamp, cuenta, max, min, media, stdev, p99);
#	$1               $2        $3       $4    $5   $6    $7     $8    $9

#if [[ -z $REGION ]]; then
#    exit
#fi

if [[ $# -ne "9" ]]; then
    echo "numero de parametros incorrectos, esperado:"
    echo
    echo "sac, sic, cat, region, timestamp, cuenta, max, min, media, stdev, p99"
    echo "\$1               \$2        \$3       \$4    \$5   \$6    \$7     \$8    \$9"
    echo 
    echo "recibidos ($#)"
    echo
    echo $0 $*
    echo
    exit
fi

#REGION=${REGION:-"NA"}
SACSICCAT=$1
REGION=$2
TIMESTAMP=$3


#if [[ "$SACSICCAT" == *"_002" ]] || [[ "$SACSICCAT" == *"_008" ]] || [[ "$SACSICCAT" == *"_034" ]]; then
#    # no vamos a generar rrds para categorias distintas de 1 y 48
#    exit
#fi


#echo `date +"%Y/%m/%d %H:%M:%S"` "REPLACE INTO availability2 (sac_sic_cat,region,timestamp,cuenta,max,min,media,stdev,p99,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,$4,$5,$6,$7,$8,now());" >> /tmp/t.t
#exit
            
echo `date +"%Y/%m/%d %H:%M:%S"` rrdtool update /var/www/lighttpd/cocirv41/rrds3/${REGION}_${SACSICCAT}.rrd ${TIMESTAMP}:$5:$6:$7:$8:$9 >> /home/eval/cocir/logs/${REGION}_${SACSICCAT}_v3.log 2>&1
rrdtool update /var/www/lighttpd/cocirv41/rrds3/${REGION}_${SACSICCAT}.rrd ${TIMESTAMP}:$5:$6:$7:$8:$9

echo `date +"%Y/%m/%d %H:%M:%S"` "REPLACE INTO availability3 (sac_sic_cat,region,timestamp,cuenta,max,min,media,stdev,p99,insert_date) VALUES (\"${SACSICCAT}\",\"${REGION}\",${TIMESTAMP},$4,$5,$6,$7,$8,$9,now());" >> /home/eval/cocir/logs/${REGION}_${SACSICCAT}_v3.log 2>&1
mysql -u root cocir -e "REPLACE INTO availability3 (sac_sic_cat,region,timestamp,cuenta,max,min,media,stdev,p99,insert_date) VALUES (\"${SACSICCAT}\",\"${REGION}\",${TIMESTAMP},$4,$5,$6,$7,$8,$9,now());" >> /home/eval/cocir/logs/${REGION}_${SACSICCAT}_v3.log 2>&1
#echo `date +"%Y/%m/%d %H:%M:%S"` "REPLACE INTO availability (sac_sic_cat,region,timestamp,cuenta,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,now());" >> /home/eval/cocir/logs/${REGION}_${1}.sql
#mysql -u root cocir -e "REPLACE INTO availability (sac_sic_cat,region,timestamp,cuenta,insert_date) VALUES (\"$1\",\"${REGION}\",$2,$3,now());" >> /home/eval/cocir/logs/${REGION}_${1}.log 2>&1
