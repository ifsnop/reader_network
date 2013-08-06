
CUR_DIR=`pwd`

#gcc -Wall -g -o bin/subscriber_SOL src/subscriber.c -L${CUR_DIR}/libs/lib/ -I${CUR_DIR}/include/ -I${CUR_DIR}/libs/include/ -lm -lconfig -ldebug -lsocket -lnsl -DSOLARIS

gcc -Wall -g -o bin/reader_network_SOL src/reader_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c src/crc32.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c -L${CUR_DIR}/libs/lib/ -I${CUR_DIR}/include/ -I${CUR_DIR}/libs/include/ -lm -lconfig -ldebug -lsocket -lnsl -DSOLARIS
exit
#gcc -Wall -g -o bin/client src/client.c src/sacsic.c src/helpers.c src/startup.c -L/aplic/reader/conversor_asterix0.45.1/libs/lib/ -Iinclude -Ilibs/include -lm -lconfig -ldebug -lsocket -lnsl -v

gcc -Wall -g -Iinclude -o bin/cleanast_SOL src/cleanast.c

gcc -Wall -g -Iinclude -lnsl -lsocket -o bin/memresp_SOL src/memresp/memresp.c -DSOLARIS


#gcc -Wall -g -o bin/reader_network src/reader_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c src/crc32.c src/red_black_tree.c src/red_black_tree_misc.c src/red_black_tree_stack.c -L/aplic/reader/distrib/src/conversor_asterix0.44/libs/lib/ -Iinclude -Ilibs/include -lm -lconfig -ldebug -lsocket -lnsl -v
