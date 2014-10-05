#!/bin/bash
NDK=/home/pallab/Documents/adt-bundle-linux-x86_64-20140702/ndk
SYSROOT=$NDK/platforms/android-19/arch-arm
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64
OPENSSL_DIR=/home/pallab/Documents/Development/bd-tv/Try2/openssl-android


ln -sf librtmp/librtmp.so.1 librtmp.so
ln -s $SYSROOT/usr/lib/crtbegin_dynamic.o:
ln -s $SYSROOT/usr/lib/crtend_android.o
ln -s $SYSROOT/usr/lib/crtbegin_dynamic.o

$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$OPENSSL_DIR/include -isysroot $SYSROOT -I$SYSROOT/ -DRTMPDUMP_VERSION=\"v2.4\" -c -o rtmpdump.o rtmpdump.c
$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -L$OPENSSL_DIR/libs/armeabi -L$SYSROOT/usr/lib  -o rtmpdump rtmpdump.o -Llibrtmp -lrtmp -lssl -lcrypto -lz
