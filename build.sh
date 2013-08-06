#!/bin/bash

if [ ! -d libs/libconfig-0.1.5 ] || [ ! -d libs/libdebug-0.4.2 ]
then
    cd libs
    ./build.sh
    echo OK OK OK OK OK OK OK OK OK
    cd ..
fi

params="-Wall -Iinclude/ -Ilibs/libdebug-0.4.2/tmp/usr/local/include -Ilibs/libconfig-0.1.5 -Llibs/libconfig-0.1.5 -Llibs/libdebug-0.4.2/src -lm -lconfig -ldebug -static -DLINUX"

gcc -Wall -g -Iinclude/ src/memresp/memresp_parse.c -o bin/memresp_parse -DLINUX

gcc -Wall -Iinclude/ src/cat10to48/convert2.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c -lm -o bin/convert2

echo reader_network
gcc -Wall -g -o bin/reader_network src/reader_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c src/crc32.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c $params
#strip bin/reader_network
exit
echo client_time
gcc -g -o bin/client_time src/client_time.c src/sacsic.c src/helpers.c src/startup.c $params
#strip bin/client_time

exit
gcc -Wall -Iinclude/ src/cleanast.c -o bin/cleanast
gcc -Wall -Iinclude/ src/memresp/memresp.c -o bin/memresp -DLINUX
gcc -Wall -Iinclude/ src/cmpclock.c -o bin/cmpclock

echo client_rrd
gcc -g -o bin/client_rrd src/client_rrd.c src/sacsic.c src/helpers.c $params
#strip bin/client_rrd

echo client_rrd2
gcc -g -o bin/client_rrd2 src/client_rrd2.c src/sacsic.c src/helpers.c $params
#strip bin/client_rrd

echo client
gcc -g -o bin/client src/client.c src/sacsic.c src/helpers.c src/startup.c $params
#strip bin/client

echo client_filter
gcc -g -o bin/client_filter src/client_filter.c src/sacsic.c src/helpers.c src/startup.c $params


echo repeater_network
gcc -Wall -g -o bin/repeater_network src/repeater_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c $params
#strip bin/reader_network

