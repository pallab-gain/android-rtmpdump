One step at a time.... 
Commits will be victorious  

Build openssl for android :

git clone https://github.com/guardianproject/openssl-android.git
cd openssl-android

#goto jni folder, commented out "NDK_TOOLCHAIN_VERSION=4.4.3" from Application.mk
#now run
ndk-build

Build rtmpdump for android :

git clone git://git.ffmpeg.org/rtmpdump
copy librtmpbuildscript.sh into librtmp folder
cd librtmp
sh librtmpbuildscript.sh
cd ..
sh buildscript.sh
