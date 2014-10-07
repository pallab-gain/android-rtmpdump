#include <jni.h>
#include "nativeutils_MyRtmp.h"

#include <android/log.h>

#define _FILE_OFFSET_BITS	64

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <signal.h>		// to catch Ctrl-C
#include <getopt.h>

#ifdef WIN32
#define fseeko fseeko64
#define ftello ftello64
#include <io.h>
#include <fcntl.h>
#define	SET_BINMODE(f)	setmode(fileno(f), O_BINARY)
#else
#define	SET_BINMODE(f)
#endif

#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE		2
#define RD_NO_CONNECT		3

#define DEF_TIMEOUT	30	/* seconds */
#define DEF_BUFTIME	(10 * 60 * 60 * 1000)	/* 10 hours default */
#define DEF_SKIPFRM	0

int InitSockets() {
	return TRUE;
}

inline void CleanupSockets() {
	return;
}
#ifdef _DEBUG
uint32_t debugTS = 0;
int pnum = 0;

FILE *netstackdump = 0;
FILE *netstackdump_read = 0;
#endif

uint32_t nIgnoredFlvFrameCounter = 0;
uint32_t nIgnoredFrameCounter = 0;
#define MAX_IGNORED_FRAMES	50

FILE *file = 0;

void sigIntHandler(int sig) {
	RTMP_ctrlC = TRUE;
//	RTMP_LogPrintf("Caught signal: %d, cleaning up, just a second...\n", sig);
	// ignore all these signals now and let the connection close
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
#ifndef WIN32
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
#endif
}

jstring Java_nativeutils_MyRtmp_getStringFromNative(JNIEnv *env, jclass jc) {
	return (*env)->NewStringUTF(env, "Hello World");
}
