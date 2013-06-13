#!/bin/bash

# Ensure that libconfig and libdebug libraries are compiled,
# the will be inserted as static libs inside our binaries,
# to ensure they are portable.

#if [ ! -d libs/libconfig-0.1.5 ] || [ ! -d libs/libdebug-0.4.2 ]
#then
#    cd libs
#    ./build.sh
#    echo OK OK OK OK OK OK OK OK OK
#    cd ..
#fi
params="-Wall -lm -lz -ldl -static -lpthread -lrt -DLINUX \
    -Iinclude/ \
    -Ilibs/lib \
    -Llibs/lib \
    -Ilibs/libconfig-0.1.5 \
    -Llibs/libconfig-0.1.5 \
    -Ilibs/libdebug-0.4.2/tmp/usr/local/include \
    -Llibs/libdebug-0.4.2/src \
    -Ilibs/curl-7.24.0/tmp/usr/local/include"

params32="-m32 -lconfig32 -ldebug32 -lcurl32 -lz32 $params"
params64="-m64 -lconfig64 -ldebug64 -lcurl64 -lz64 $params"

echo reader_reader32
gcc -Wall -g -o bin/reader_network src/reader_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c src/crc32.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c $params32

#echo reader_reader64
#gcc -Wall -g -o bin/reader_network64 src/reader_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c src/crc32.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c $params64

#echo reader_rrd
#gcc -Wall -g -o bin/reader_rrd src/reader_rrd.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c src/crc32.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c $params32 -DCLIENT_RRD

echo reader_rrd3
gcc -Wall -g -o bin/reader_rrd3 src/reader_rrd3.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c src/crc32.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c $params32 -DCLIENT_RRD

echo client_time
gcc -g -o bin/client_time src/client_time.c src/sacsic.c src/helpers.c src/startup.c $params32

echo client
gcc -g -o bin/client src/client.c src/sacsic.c src/helpers.c src/startup.c $params32

echo cleanast
gcc -Wall -Iinclude/ src/cleanast.c -o bin/cleanast

gcc -Wall -Iinclude/ src/memresp/memresp.c -o bin/memresp -DLINUX
gcc -Wall -Iinclude/ src/memresp/memresps.c -o bin/memresps -DLINUX

exit

exit

# old binaries


#gcc -Wall -Iinclude/ src/cmpclock.c -o bin/cmpclock
#echo client_rrd
#gcc -g -o bin/client_rrd src/client_rrd.c src/sacsic.c src/helpers.c $params
#strip bin/client_rrd
#echo client_rrd2
#gcc -g -o bin/client_rrd2 src/client_rrd2.c src/sacsic.c src/helpers.c $params
#strip bin/client_rrd
#echo client
#gcc -g -o bin/client src/client.c src/sacsic.c src/helpers.c src/startup.c $params
#strip bin/client
#echo client_filter
#gcc -g -o bin/client_filter src/client_filter.c src/sacsic.c src/helpers.c src/startup.c $params
#echo repeater_network
#gcc -Wall -g -o bin/repeater_network src/repeater_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c $params
#strip bin/reader_network

