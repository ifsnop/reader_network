#!/bin/bash

# check if we can build 32bits binaries
cat > /tmp/conftest.c << _LT_EOF
int main() { return 0;}
_LT_EOF
gcc -m32 /tmp/conftest.c -o /tmp/conftest > /dev/null 2>&1
can32bits=$?
rm /tmp/conftest.c /tmp/conftest 2> /dev/null

arch=""
additionallibs=""
MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
    destarchs="64"
    if [[ ${can32bits} -eq 0 ]]; then
	destarchs="64 32"
    fi
elif [ ${MACHINE_TYPE} == 'sun4u' ]; then
    destarchs="32"
    arch="_sparc"
    additionallibs="-lsocket -lnsl"
else
    destarchs="32"
fi

#rm libs/lib*
#rm -rf 3rdparty/tmp

for destarch in $destarchs; do

    gccopts="-Wl,-Bstatic -m${destarch} -O2 -pipe -Iinclude -Llibs -Bstatic \
	-Wall -Wno-trigraphs -fno-strict-aliasing -fno-common -ggdb -Wl,-Bdynamic"

    ## quarrantined options:
    ## "-fPIC" (only works for shared libraries, not used)
    ## useful options:
    ## "-O2" to enable optimizations
    ## "-ggdb" to enable debug symbols
    ## "-DDEBUG_LOG -DDEBUG_MEM" to enable debuging of memory allocation
    ## unknown what they do, even if the are used "-DGETHOSTBYNAME -DGETSERVBYNAME"

    if [[ ! -f libs/libdebug${destarch}${arch}.a ]]; then
	gcc $gccopts -c -o obj/log${destarch}.o src/libdebug/log.c
	gcc $gccopts -c -o obj/memory${destarch}.o src/libdebug/memory.c
	gcc $gccopts -c -o obj/hex${destarch}.o src/libdebug/hex.c
	ar crv libs/libdebug${destarch}${arch}.a obj/log${destarch}.o obj/memory${destarch}.o obj/hex${destarch}.o > /dev/null
    fi

    if [[ ! -f libs/libconfig${destarch}${arch}.a ]]; then
	gcc $gccopts -c -o obj/scan${destarch}.o src/libconfig/scan.c
	gcc $gccopts -c -o obj/parse${destarch}.o src/libconfig/parse.c
	gcc $gccopts -c -o obj/config${destarch}.o src/libconfig/config.c
	ar crv libs/libconfig${destarch}${arch}.a obj/scan${destarch}.o obj/parse${destarch}.o obj/config${destarch}.o > /dev/null
    fi

    if [[ ! -f libs/libz${destarch}${arch}.a ]]; then
	pushd . > /dev/null
	rm -r 3rdparty/tmp/zlib-1.2.8 2> /dev/null
	mkdir 3rdparty/tmp/ 2> /dev/null
	cd 3rdparty/tmp > /dev/null
        gzip -dc ../zlib-1.2.8.tar.gz | tar xf - 
        cd ../.. > /dev/null

	DESTDIR=`pwd`/3rdparty/tmp/
	cd $DESTDIR/zlib-1.2.8 > /dev/null
        CFLAGS="-m${destarch}" ./configure --static > /tmp/build${destarch}${arch}.log 2>&1
        make install DESTDIR=$DESTDIR >> /tmp/build${destarch}${arch}.log 2>&1
	popd > /dev/null
	cp $DESTDIR/usr/local/lib/libz.a libs/libz${destarch}${arch}.a
    fi

    if [[ ! -f libs/libcurl${destarch}${arch}.a ]]; then
	pushd . > /dev/null
	rm -r 3rdparty/tmp/curl-7.30.0 2> /dev/null
	mkdir 3rdparty/tmp/ 2> /dev/null
        cd 3rdparty/tmp > /dev/null
	bzip2 -dc ../curl-7.30.0.tar.bz2 | tar xf -
        cd ../.. > /dev/null

	DESTDIR=`pwd`/3rdparty/tmp/
	cd $DESTDIR/curl-7.30.0 > /dev/null
	export CFLAGS="-m${destarch}"
	./configure --enable-static --without-libidn --without-librtmp \
            --disable-shared --disable-ssl \
	    --disable-ipv6 --disable-rtsp --disable-dict --disable-gopher --disable-https \
	    --disable-telnet --disable-smtp --disable-smtps --disable-imap --disable-imaps \
	    --disable-pop3 --disable-pop3s --disable-ftps --without-ssl --without-polarssl \
	    --disable-ldap --disable-ldaps --disable-debug --disable-ntlm-wb \
	    --disable-tls-srp --prefix=/usr/local/${destarch}${arch} >> /tmp/build${destarch}${arch}.log 2>&1
	make install DESTDIR=$DESTDIR >> /tmp/build${destarch}${arch}.log 2>&1
        popd > /dev/null
	cp $DESTDIR/usr/local/${destarch}${arch}/lib/libcurl.a libs/libcurl${destarch}${arch}.a
    fi

    rnopts="-lm -ldl -lpthread -static-libgcc ${additionallibs} \
	-lconfig${destarch}${arch} -ldebug${destarch}${arch} \
	-lcurl${destarch}${arch} -lz${destarch}${arch} -lrt \
	-I3rdparty/tmp/usr/local/${destarch}${arch}/include"

    rncfiles="src/asterix.c src/sacsic.c src/helpers.c \
	src/startup.c src/crc32.c src/red_black_tree.c \
	src/red_black_tree_misc.c src/red_black_tree_stack.c \
	src/md5.c"

    gcc $gccopts -o bin/reader_network${destarch}${arch} $rncfiles src/reader_network.c $rnopts
    #strip bin/reader_network${destarch} 2> /dev/null
    gcc $gccopts -DCLIENT_RRD -o bin/reader_rrd3${destarch}${arch} $rncfiles src/reader_rrd3.c $rnopts -I/usr/include/mysql -DBIG_JOINS=1 -L/usr/lib/x86_64-linux-gnu -lmysqlclient
#`mysql_config --libs | cut -d ' ' -f1`
    gcc $gccopts -o bin/client_time${destarch}${arch} src/client_time.c src/sacsic.c src/helpers.c src/startup.c $rnopts
    gcc $gccopts -o bin/client${destarch}${arch} src/client.c src/sacsic.c src/helpers.c src/startup.c $rnopts
    gcc $gccopts -o bin/cleanast${destarch}${arch} src/utils/cleanast.c $rnopts
    gcc $gccopts -o bin/filtersacsic${destarch}${arch} src/utils/filtersacsic.c $rnopts
    gcc $gccopts -o bin/joingps${destarch}${arch} src/utils/joingps.c $rnopts
    gcc $gccopts -o bin/memresps${destarch}${arch} src/memresp/memresps.c $rnopts

done

#gcc -Wall -Iinclude/ src/memresp/memresp.c -o bin/memresp -DLINUX
#gcc -Wall -Iinclude/ src/memresp/memresps.c -o bin/memresps -DLINUX

ls -la bin/

exit

#old binaries

#gcc -Wall -Iinclude/ src/cmpclock.c -o bin/cmpclock
#gcc -g -o bin/client_rrd src/client_rrd.c src/sacsic.c src/helpers.c $params
#gcc -g -o bin/client_rrd2 src/client_rrd2.c src/sacsic.c src/helpers.c $params
#gcc -g -o bin/client src/client.c src/sacsic.c src/helpers.c src/startup.c $params
#gcc -g -o bin/client_filter src/client_filter.c src/sacsic.c src/helpers.c src/startup.c $params
#gcc -Wall -g -o bin/repeater_network src/repeater_network.c src/asterix.c src/sacsic.c src/helpers.c src/startup.c $params

