rm distrib/rn075.tar.bz2
tar cvfj distrib/rn075.tar.bz2 \
    bin/client* \
    bin/reader_network* \
    bin/reader_rrd3* bin/cleanast* bin/scripts/* bin/conf/* bin/filter*
cp distrib/rn075.tar.bz2 /home/eval
