#!/bin/bash
NDK=/home/pallab/Documents/adt-bundle-linux-x86_64-20140702/ndk
SYSROOT=$NDK/platforms/android-19/arch-arm
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64
OPENSSL_DIR=`pwd`/../openssl-android
CUR_DIR=`pwd`
FILENAME=HelloJNI
SONAME=hellojni

make clean

ln -sf librtmp/librtmp.so.1 librtmp.so
ln -s $SYSROOT/usr/lib/crtend_so.o
ln -s $SYSROOT/usr/lib/crtbegin_so.o
ln -s $SYSROOT/usr/lib/crtbegin_dynamic.o
ln -s $SYSROOT/usr/lib/crtend_android.o



$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$CUR_DIR -I$OPENSSL_DIR/include  -isysroot $SYSROOT -I$SYSROOT/ -DRTMPDUMP_VERSION=\"v2.4\" -c -o $FILENAME.o $FILENAME.c
$TOOLCHAIN/bin/arm-linux-androideabi-gcc -Wall -marm -I$CUR_DIR -L$OPENSSL_DIR/libs/armeabi -L$SYSROOT/usr/lib  -o $FILENAME $FILENAME.o -Llibrtmp -lrtmp -lssl -lcrypto -lz


$TOOLCHAIN/bin/arm-linux-androideabi-ar rs $FILENAME.a $FILENAME.o
$TOOLCHAIN/bin/arm-linux-androideabi-gcc -shared -Wl,-soname,lib$SONAME.so.1  -L$OPENSSL_DIR/libs/armeabi -L$SYSROOT/usr/lib  -o lib$SONAME.so $FILENAME.o  -lssl -lcrypto -lz 
