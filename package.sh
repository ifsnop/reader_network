rm distrib/rn078.tar.bz2
tar cvfj distrib/rn078.tar.bz2 \
    bin/client* \
    bin/reader_network* \
    bin/reader_rrd3* bin/cleanast* bin/scripts/* bin/conf/* bin/filter*
cp distrib/rn078.tar.bz2 /home/eval
scp distrib/rn078.tar.bz2 eval@nemo:.
scp distrib/rn078.tar.bz2 eval@carol:.
scp distrib/rn078.tar.bz2 eval@coral:.
