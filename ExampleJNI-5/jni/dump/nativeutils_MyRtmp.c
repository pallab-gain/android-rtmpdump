#include <jni.h>
#include <stdlib.h>
#include "nativeutils_MyRtmp.h"
#include <android/log.h>

#define TAG "Xerxes-JNI-Call"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define _FILE_OFFSET_BITS	64

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

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
char *temp_filepath = 0;
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
void wait(double seconds) {
	double endtime = clock() + (seconds * CLOCKS_PER_SEC);
	while (clock() < endtime) {
		;
	}
}
#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)
#define STR2SingleAVAL(av,str)	av = str

static JavaVM * gJavaVM;
static jobject _gObject;
static jclass _gClass;

static int Download(
		RTMP * rtmp, // connected RTMP object
		FILE * file, uint32_t dSeek, uint32_t dStopOffset, double duration,
		int bResume, char *metaHeader, uint32_t nMetaHeaderSize,
		char *initialFrame, int initialFrameType, uint32_t nInitialFrameSize,
		int nSkipKeyFrames, int bStdoutMode, int bLiveStream, int bHashes,
		int bOverrideBufferTime, uint32_t bufferTime, double *percent) {
	JNIEnv * g_env;
	int isAttached = FALSE;
	int getEnvStat = (*gJavaVM)->GetEnv(gJavaVM, (void **) &g_env,
			JNI_VERSION_1_6);
	jmethodID method_id;
	if (getEnvStat == JNI_EDETACHED) {
		LOGE("GetEnv: not attached");
		int status = (*gJavaVM)->AttachCurrentThread(gJavaVM, &g_env, NULL);
		if (status != 0) {
			LOGE("Failed to attach");
			return RD_FAILED;
		}
		isAttached = TRUE;
	} else if (getEnvStat == JNI_OK) {
		LOGE("NICE ready to call a callback");
		method_id = (*g_env)->GetStaticMethodID(g_env, _gClass, "callback",
				"(I)V");
		if (!method_id) {
			LOGE("failed to get method id");
			if (isAttached) {
				if (isAttached == TRUE) {
					(*gJavaVM)->DetachCurrentThread(gJavaVM);
				}
				return RD_FAILED;
			}
		}
	} else if (getEnvStat == JNI_EVERSION) {
		LOGE("GetEnv: version not supported");
		return RD_FAILED;
	}

//	START ( DOWNLOAD THE STREAM )
	{
		int32_t now, lastUpdate;
		int bufferSize = 64 * 1024;
		char *buffer = (char *) malloc(bufferSize);
		int nRead = 0;
		off_t size = ftello(file);
		unsigned long lastPercent = 0;

		rtmp->m_read.timestamp = dSeek;

		*percent = 0.0;

		//PROBABLY REMOVING IT
		if (bResume && nInitialFrameSize > 0)
			rtmp->m_read.flags |= RTMP_READ_RESUME;

		rtmp->m_read.initialFrameType = initialFrameType;
		rtmp->m_read.nResumeTS = dSeek;
		rtmp->m_read.metaHeader = metaHeader;
		rtmp->m_read.initialFrame = initialFrame;
		rtmp->m_read.nMetaHeaderSize = nMetaHeaderSize;
		rtmp->m_read.nInitialFrameSize = nInitialFrameSize;

		now = RTMP_GetTime();
		lastUpdate = now - 1000;
		off_t upto = (off_t) 1024 * 1024 * 2; //5 mb file
		do {
			nRead = RTMP_Read(rtmp, buffer, bufferSize);
//			if (upto < size) {
//				LOGE("I just excessed 5 mb");
//				if (file != 0) {
//					fclose(file);
//					file = fopen(temp_filepath, "wb");
//					size = ftello(file);
//					(*g_env)->CallStaticVoidMethod(g_env, _gClass, method_id,0);
//				}else{
//					file = fopen(temp_filepath, "wb");
//					size = ftello(file);
//					(*g_env)->CallStaticVoidMethod(g_env, _gClass, method_id,0);
//				}
//
//			}

			if (nRead > 0) {
				if (fwrite(buffer, sizeof(unsigned char), nRead, file)
						!= (size_t) nRead) {
					LOGE("%s: Failed writing, exiting!", __FUNCTION__);
					free(buffer);
					return RD_FAILED;
				}
				size += nRead;

				if (duration <= 0) // if duration unknown try to get it from the stream (onMetaData)
					duration = RTMP_GetDuration(rtmp);

				if (duration > 0) {
					// make sure we claim to have enough buffer time!
					if (!bOverrideBufferTime
							&& bufferTime < (duration * 1000.0)) {
						bufferTime = (uint32_t)(duration * 1000.0) + 5000; // extra 5sec to make sure we've got enough
						RTMP_SetBufferMS(rtmp, bufferTime);
						RTMP_UpdateBufferMS(rtmp);
					}
					*percent = ((double) rtmp->m_read.timestamp)
							/ (duration * 1000.0) * 100.0;
					*percent = ((double) (int) (*percent * 10.0)) / 10.0;
					if (bHashes) {
						if (lastPercent + 1 <= *percent) {
							lastPercent = (unsigned long) *percent;
						}
					} else {
						now = RTMP_GetTime();
						if (abs(now - lastUpdate) > 200) {
							lastUpdate = now;
						}
					}
				} else {
					now = RTMP_GetTime();
					if (abs(now - lastUpdate) > 200) {
						lastUpdate = now;
					}
				}
			}
		} while (!RTMP_ctrlC && nRead > -1 && RTMP_IsConnected(rtmp));
		free(buffer);

		if (nRead < 0)
			nRead = rtmp->m_read.status;

		/* Final status update */
		if (!bHashes) {
			if (duration > 0) {
				*percent = ((double) rtmp->m_read.timestamp)
						/ (duration * 1000.0) * 100.0;
				*percent = ((double) (int) (*percent * 10.0)) / 10.0;
			}
		}

		LOGE("RTMP_Read returned: %d", nRead);
		if (bResume && nRead == -2) {
			RTMP_LogPrintf("Couldn't resume FLV file, try --skip %d\n\n",
					nSkipKeyFrames + 1);
			return RD_FAILED;
		}

		if (nRead == -3)
			return RD_SUCCESS;

		if ((duration > 0 && *percent < 99.9) || RTMP_ctrlC || nRead < 0
				|| RTMP_IsTimedout(rtmp)) {
			return RD_INCOMPLETE;
		}

	}
//	END ( DOWNLOAD THE STREAM)
	return RD_SUCCESS;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv *env;
	gJavaVM = vm;
	LOGE("JNI_OnLoad called");
	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGE("Failed to get the environment using GetEnv()");
		return -1;
	}
	return JNI_VERSION_1_6;
}
void JNI_OnUnload(JavaVM *vm, void *reserved) {
	LOGE("JNI OnUnload called");
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
		// Something is wrong but nothing we can do about this :(
		return;
	} else {
		// delete global references so the GC can collect them
		if (_gClass != NULL) {
			(*env)->DeleteGlobalRef(env, _gClass);
		}
		if (_gObject != NULL) {
			(*env)->DeleteGlobalRef(env, _gObject);
		}
	}
}
JNIEXPORT jint JNICALL Java_nativeutils_MyRtmp_CallMain(JNIEnv *env,
		jobject job, jstring _rtmpUrl, jstring _appName, jstring _SWFUrl,
		jstring _pageUrl, jstring _playPath, jstring _filePath) {
	//BEGIN (CACHE IT)
	_gObject = (jobject)(*env)->NewGlobalRef(env, job);
	jclass clazz = (*env)->GetObjectClass(env, job);
	_gClass = (jclass)(*env)->NewGlobalRef(env, clazz);
	//END (CACHE IT)
	const char *rtmp_url = (*env)->GetStringUTFChars(env, _rtmpUrl, 0);
	char *app_name = (char *) (*env)->GetStringUTFChars(env, _appName, 0);
	char *swf_url = (char *) (*env)->GetStringUTFChars(env, _SWFUrl, 0);
	char *page_url = (char *) (*env)->GetStringUTFChars(env, _pageUrl, 0);
	char *play_path = (char *) (*env)->GetStringUTFChars(env, _playPath, 0);
	char *file_path = (char *) (*env)->GetStringUTFChars(env, _filePath, 0);

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
	//write to file
	STR2SingleAVAL(flvFile, file_path);
	//cacheing it for other purpose
	STR2SingleAVAL(temp_filepath, file_path);
	bStdoutMode = FALSE;

	if (port == 0) {
		if (protocol & RTMP_FEATURE_SSL)
			port = 443;
		else if (protocol & RTMP_FEATURE_HTTP)
			port = 80;
		else
			port = 1935;
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
//	if (!file) {
	if (bStdoutMode) {
		file = stdout;
		SET_BINMODE(file);
	} else {
		file = fopen(flvFile, "wb");
		if (file == 0) {
			LOGE("Cannot write to file sorry!");
			return RD_FAILED;
		}
	}
//	}
//END (RTMP INI )

//START ( TRYING TO CONNECT )
	{
		LOGE("Setting buffer time %d ms", bufferTime);
		RTMP_SetBufferMS(&rtmp, bufferTime);

		LOGE("Connecting....");
		if (!RTMP_Connect(&rtmp, NULL)) {
			LOGE("Failed to connect!");
			nStatus = RD_FAILED;
		}
		LOGE("Connected!");
		if (!RTMP_ConnectStream(&rtmp, dSeek)) {
			LOGE("Failed to connect stream ");
			nStatus = RD_FAILED;
		}
	}
	//END ( TRYING TO CONNECT )
	if (nStatus == RD_SUCCESS) {
		//A SUCCESSFUL CONNECTION
		//Go get the stream
		RTMP_ctrlC = FALSE;

		nStatus = Download(&rtmp, file, dSeek, dStopOffset, duration, bResume,
				metaHeader, nMetaHeaderSize, initialFrame, initialFrameType,
				nInitialFrameSize, nSkipKeyFrames, bStdoutMode, bLiveStream,
				bHashes, bOverrideBufferTime, bufferTime, &percent);

		free(initialFrame);
		initialFrame = NULL;
	}

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
	(*env)->ReleaseStringUTFChars(env, _filePath, file_path);
	return nStatus;
}

JNIEXPORT void JNICALL Java_nativeutils_MyRtmp_stopNativeRecording
(JNIEnv *env, jobject job) {
	RTMP_ctrlC = TRUE;
}
