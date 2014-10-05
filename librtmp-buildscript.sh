#!/bin/bash
NDK=/home/pallab/Documents/adt-bundle-linux-x86_64-20140702/ndk
SYSROOT=$NDK/platforms/android-19/arch-arm
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64
OPENSSL_DIR=/home/pallab/Documents/Development/bd-tv/Try2/openssl-android

ln -s $SYSROOT/usr/lib/crtend_so.o
ln -s $SYSROOT/usr/lib/crtbegin_so.o

$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$OPENSSL_DIR/include -isysroot $SYSROOT -I$SYSROOT/ -DRTMPDUMP_VERSION=\"v2.4\" -DUSE_OPENSSL -fPIC -c -o rtmp.o rtmp.c

$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$OPENSSL_DIR/include -isysroot $SYSROOT -I$SYSROOT/ -DRTMPDUMP_VERSION=\"v2.4\" -DUSE_OPENSSL   -fPIC   -c -o log.o log.c
$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$OPENSSL_DIR/include -isysroot $SYSROOT -I$SYSROOT/ -DRTMPDUMP_VERSION=\"v2.4\" -DUSE_OPENSSL   -fPIC   -c -o amf.o amf.c

$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$OPENSSL_DIR/include -isysroot $SYSROOT -I$SYSROOT/ -DRTMPDUMP_VERSION=\"v2.4\" -DUSE_OPENSSL   -fPIC   -c -o hashswf.o hashswf.c
$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$OPENSSL_DIR/include -isysroot $SYSROOT -I$SYSROOT/ -DRTMPDUMP_VERSION=\"v2.4\" -DUSE_OPENSSL   -fPIC   -c -o parseurl.o parseurl.c

$TOOLCHAIN/bin/arm-linux-androideabi-ar rs librtmp.a rtmp.o log.o amf.o hashswf.o parseurl.o

$TOOLCHAIN/bin/arm-linux-androideabi-gcc -shared -Wl,-soname,librtmp.so.1  -L$OPENSSL_DIR/libs/armeabi -L$SYSROOT/usr/lib  -o librtmp.so.1 rtmp.o log.o amf.o hashswf.o parseurl.o  -lssl -lcrypto -lz 

# ln -sf librtmp.so.1 librtmp.so
# $TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall  -L$OPENSSL_DIR/libs/armeabi -L$SYSROOT/usr/lib  -o rtmpdump rtmpdump.o -Llibrtmp -lrtmp -lssl -lcrypto -lz