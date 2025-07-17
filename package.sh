rm distrib/rn080.tar.bz2
tar cvfj distrib/rn080.tar.bz2 \
    bin/client* \
    bin/reader_network* \
    bin/reader_rrd3* bin/cleanast* bin/scripts/* bin/conf/* bin/filter*
cp distrib/rn080.tar.bz2 /home/eval
scp distrib/rn080.tar.bz2 eval@nemo_192:.
scp distrib/rn080.tar.bz2 eval@carol_192:.
scp distrib/rn080.tar.bz2 eval@coral_192:.
