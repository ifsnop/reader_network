#/!bin/bash

rm -r libconfig-0.1.5 2>/dev/null
rm -r libdebug-0.4.2 2>/dev/null

#tar xvfz libconfig_0.1.5.tar.gz
#tar xvfz libdebug_0.4.2.tar.gz

gzip -dc libconfig_0.1.5.tar.gz | tar xvf -
gzip -dc libdebug_0.4.2.tar.gz | tar xvf -

cd libdebug-0.4.2
make
make install DESTDIR=tmp
cd ..

cd libconfig-0.1.5
make prefix=../libdebug-0.4.2/tmp/usr/local
cd ..

cd  curl-7.24.0
./configure --enable-static --without-libidn --disable-shared --disable-ssl --disable-ipv6 --disable-rtsp --disable-dict --disable-gopher --disable-https --disable-telnet --disable-smtp --disable-smtps --disable-imap --disable-imaps --disable-pop3 --disable-pop3s --disable-ftps --without-ssl --without-polarssl
make install DESTDIR=`pwd`/tmp
cp tmp/usr/local/lib/libcurl.a ../lib/libcurl64.a

export CFLAGS=-m32
./configure --enable-static --without-libidn --disable-shared --disable-ssl --disable-ipv6 --disable-rtsp --disable-dict --disable-gopher --disable-https --disable-telnet --disable-smtp --disable-smtps --disable-imap --disable-imaps --disable-pop3 --disable-pop3s --disable-ftps --without-ssl --without-polarssl
make install DESTDIR=`pwd`/tmp
cp tmp/usr/local/lib/libcurl.a ../lib/libcurl32.a


cd ..

