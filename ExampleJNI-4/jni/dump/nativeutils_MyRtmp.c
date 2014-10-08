#include <jni.h>
#include <stdlib.h>
#include "nativeutils_MyRtmp.h"
#include <android/log.h>

#define TAG "RTMPDUMP MYNATIVE"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define _FILE_OFFSET_BITS	64

#include <string.h>
#include <math.h>
#include <stdio.h>

#include <signal.h>		// to catch Ctrl-C
#include <getopt.h>

#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

#ifdef WIN32
#define fseeko fseeko64
#define ftello ftello64
#include <io.h>
#include <fcntl.h>
#define	SET_BINMODE(f)	setmode(fileno(f), O_BINARY)
#else
#define	SET_BINMODE(f)
#endif

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE		2

#define DEF_TIMEOUT	30	/* seconds */
#define DEF_BUFTIME	(10 * 60 * 60 * 1000)	/* 10 hours default */
#define DEF_SKIPFRM	0

// starts sockets
int InitSockets() {
	return TRUE;
}

inline void CleanupSockets() {
	return;
}

uint32_t nIgnoredFlvFrameCounter = 0;
uint32_t nIgnoredFrameCounter = 0;
#define MAX_IGNORED_FRAMES	50

FILE *file = 0;

void sigIntHandler(int sig) {
	RTMP_ctrlC = TRUE;
	RTMP_LogPrintf("Caught signal: %d, cleaning up, just a second...\n", sig);
	// ignore all these signals now and let the connection close
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
#ifndef WIN32
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
#endif
}

#define HEX2BIN(a)      (((a)&0x40)?((a)&0xf)+9:((a)&0xf))
int hex2bin(char *str, char **hex) {
	char *ptr;
	int i, l = strlen(str);

	if (l & 1)
		return 0;

	*hex = malloc(l / 2);
	ptr = *hex;
	if (!ptr)
		return 0;

	for (i = 0; i < l; i += 2)
		*ptr++ = (HEX2BIN(str[i]) << 4) | HEX2BIN(str[i+1]);
	return l / 2;
}
#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

JNIEXPORT jint JNICALL Java_nativeutils_MyRtmp_CallMain(JNIEnv *env,
		jclass jcls, jstring _rtmpUrl, jstring _appName, jstring _SWFUrl,
		jstring _pageUrl, jstring _playPath) {
	const char *rtmp_url = (*env)->GetStringUTFChars(env, _rtmpUrl, 0);
	char *app_name = (*env)->GetStringUTFChars(env, _appName, 0);
	char *swf_url = (*env)->GetStringUTFChars(env, _SWFUrl, 0);
	char *page_url = (*env)->GetStringUTFChars(env, _pageUrl, 0);
	char *play_path = (*env)->GetStringUTFChars(env, _playPath, 0);

	//BEGIN (RTMP INI)
	int nStatus = RD_SUCCESS;
	double percent = 0;
	double duration = 0.0;

	int nSkipKeyFrames = DEF_SKIPFRM; // skip this number of keyframes when resuming

	int bOverrideBufferTime = FALSE; // if the user specifies a buffer time override this is true
	int bStdoutMode = TRUE; // if true print the stream directly to stdout, messages go to stderr
	int bResume = FALSE; // true in resume mode
	uint32_t dSeek = 0; // seek position in resume mode, 0 otherwise
	uint32_t bufferTime = DEF_BUFTIME;

	// meta header and initial frame for the resume mode (they are read from the file and compared with
	// the stream we are trying to continue
	char *metaHeader = 0;
	uint32_t nMetaHeaderSize = 0;

	// video keyframe for matching
	char *initialFrame = 0;
	uint32_t nInitialFrameSize = 0;
	int initialFrameType = 0;	// tye: audio or video

	AVal hostname = { 0, 0 };
	AVal playpath = { 0, 0 };
	AVal subscribepath = { 0, 0 };
	int port = -1;
	int protocol = RTMP_PROTOCOL_UNDEFINED;
	int retries = 0;
	int bLiveStream = FALSE; // is it a live stream? then we can't seek/resume
	int bHashes = FALSE; // display byte counters not hashes by default

	long int timeout = DEF_TIMEOUT; // timeout connection after 120 seconds
	uint32_t dStartOffset = 0; // seek position in non-live mode
	uint32_t dStopOffset = 0;
	RTMP rtmp = { 0 };

	AVal swfUrl = { 0, 0 };
	AVal tcUrl = { 0, 0 };
	AVal pageUrl = { 0, 0 };
	AVal app = { 0, 0 };
	AVal auth = { 0, 0 };
	AVal swfHash = { 0, 0 };
	uint32_t swfSize = 0;
	AVal flashVer = { 0, 0 };
	AVal sockshost = { 0, 0 };

#ifdef CRYPTO
	int swfAge = 30; /* 30 days for SWF cache by default */
	int swfVfy = 0;
	unsigned char hash[RTMP_SWF_HASHLEN];
#endif

	char *flvFile = 0;

	signal(SIGINT, sigIntHandler);
	signal(SIGTERM, sigIntHandler);
#ifndef WIN32
	signal(SIGHUP, sigIntHandler);
	signal(SIGPIPE, sigIntHandler);
	signal(SIGQUIT, sigIntHandler);
#endif

	RTMP_debuglevel = RTMP_LOGINFO;
	if (!InitSockets()) {
		return RD_FAILED;
	}
	/* sleep(30); */

	RTMP_Init(&rtmp);
	bLiveStream = TRUE; // no seeking or resuming possible!

	AVal parsedHost, parsedApp, parsedPlaypath;
	unsigned int parsedPort = 0;
	int parsedProtocol = RTMP_PROTOCOL_UNDEFINED;

	//LETS PARSE THE INFORMATION NECESSARY FOR A CONNECTION
	if (!RTMP_ParseURL(rtmp_url, &parsedProtocol, &parsedHost, &parsedPort,
			&parsedPlaypath, &parsedApp)) {
		LOGE("Couldn't parse the specified url ");
	} else {
		if (!hostname.av_len)
			hostname = parsedHost;
		if (port == -1)
			port = parsedPort;
		if (playpath.av_len == 0 && parsedPlaypath.av_len) {
			playpath = parsedPlaypath;
		}
		if (protocol == RTMP_PROTOCOL_UNDEFINED)
			protocol = parsedProtocol;
		if (app.av_len == 0 && parsedApp.av_len) {
			app = parsedApp;
		}
	}
	STR2AVAL(app, app_name);
#ifdef CRYPTO
	STR2AVAL(swfUrl, swf_url);
	swfVfy = 1;
#endif
	STR2AVAL(pageUrl, page_url);
	STR2AVAL(playpath, play_path);

	if (port == 0) {
		if (protocol & RTMP_FEATURE_SSL)
			port = 443;
		else if (protocol & RTMP_FEATURE_HTTP)
			port = 80;
		else
			port = 1935;
	}

	if (flvFile == 0) {
		LOGE(
				"You haven't specified an output file (-o filename), using stdout");
		bStdoutMode = TRUE;
	}

#ifdef CRYPTO
	if (swfVfy) {
		if (RTMP_HashSWF(swfUrl.av_val, &swfSize, hash, swfAge) == 0) {
			swfHash.av_val = (char *) hash;
			swfHash.av_len = RTMP_SWF_HASHLEN;
		}
	}

	if (swfHash.av_len == 0 && swfSize > 0) {
		LOGE("Ignoring SWF size, supply also the hash with --swfhash");
		swfSize = 0;
	}

	if (swfHash.av_len != 0 && swfSize == 0) {
		LOGE("Ignoring SWF hash, supply also the swf size  with --swfsize");
		swfHash.av_len = 0;
		swfHash.av_val = NULL;
	}
#endif

	if (tcUrl.av_len == 0) {
		char str[512] = { 0 };

		tcUrl.av_len = snprintf(str, 511, "%s://%.*s:%d/%.*s",
				RTMPProtocolStringsLower[protocol], hostname.av_len,
				hostname.av_val, port, app.av_len, app.av_val);
		tcUrl.av_val = (char *) malloc(tcUrl.av_len + 1);
		strcpy(tcUrl.av_val, str);
	}

	int first = 1;
	RTMP_SetupStream(&rtmp, protocol, &hostname, port, &sockshost, &playpath,
			&tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
			&flashVer, &subscribepath, dSeek, dStopOffset, bLiveStream,
			timeout);

	/* Try to keep the stream moving if it pauses on us */
	if (!bLiveStream && !(protocol & RTMP_FEATURE_HTTP)) {
		rtmp.Link.lFlags |= RTMP_LF_BUFX;
	}
	if (!file) {
		if (bStdoutMode) {
			file = stdout;
			SET_BINMODE(file);
		} else {
			LOGE("NOT SUPPORTING WRITE TO FILE! REAL TIME FOR NOW");
			return RD_FAILED;
		}
	}
	//END (RTMP INI )

	//START ( TRYING TO CONNECT )
	//END ( TRYING TO CONNECT )

	clean: LOGE("CLOSING CONNECTION");
	RTMP_Close(&rtmp);
	if (file != 0)
		fclose(file);

	CleanupSockets();

	//RELEASE THE DRAGOOOON
	(*env)->ReleaseStringUTFChars(env, _rtmpUrl, rtmp_url);
	(*env)->ReleaseStringUTFChars(env, _appName, app_name);
	(*env)->ReleaseStringUTFChars(env, _SWFUrl, swf_url);
	(*env)->ReleaseStringUTFChars(env, _pageUrl, page_url);
	(*env)->ReleaseStringUTFChars(env, _playPath, play_path);
	return nStatus;
}

