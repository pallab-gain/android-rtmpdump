/*  RTMPDump
 *  Copyright (C) 2009 Andrej Stepanchuk
 *  Copyright (C) 2009 Howard Chu
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTMPDump; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define _FILE_OFFSET_BITS	64

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <signal.h>		// to catch Ctrl-C
#include <getopt.h>

#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

#define	SET_BINMODE(f)

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE		2

#define DEF_TIMEOUT	30	/* seconds */
#define DEF_BUFTIME	(10 * 60 * 60 * 1000)	/* 10 hours default */
#define DEF_SKIPFRM	0

inline void CleanupSockets() {
	return ;
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

static const AVal av_onMetaData = AVC("onMetaData");
static const AVal av_duration = AVC("duration");
static const AVal av_conn = AVC("conn");
static const AVal av_token = AVC("token");
static const AVal av_playlist = AVC("playlist");
static const AVal av_true = AVC("true");

int Download(
		RTMP * rtmp, // connected RTMP object
		FILE * file, uint32_t dSeek, uint32_t dStopOffset, double duration,
		int bResume, char *metaHeader, uint32_t nMetaHeaderSize,
		char *initialFrame, int initialFrameType, uint32_t nInitialFrameSize,
		int nSkipKeyFrames, int bStdoutMode, int bLiveStream, int bHashes,
		int bOverrideBufferTime, uint32_t bufferTime, double *percent) // percentage downloaded [out]
{
	int32_t now, lastUpdate;
	int bufferSize = 64 * 1024;
	char *buffer = (char *) malloc(bufferSize);
	int nRead = 0;
	off_t size = ftello(file);
	unsigned long lastPercent = 0;

	rtmp->m_read.timestamp = dSeek;

	*percent = 0.0;

	if (rtmp->m_read.timestamp) {
		RTMP_Log(RTMP_LOGDEBUG, "Continuing at TS: %d ms\n",
				rtmp->m_read.timestamp);
	}

	if (bLiveStream) {
		RTMP_LogPrintf("Starting Live Stream\n");
	} else {
		// print initial status
		// Workaround to exit with 0 if the file is fully (> 99.9%) downloaded
		if (duration > 0) {
			if ((double) rtmp->m_read.timestamp >= (double) duration * 999.0) {
				RTMP_LogPrintf(
						"Already Completed at: %.3f sec Duration=%.3f sec\n",
						(double) rtmp->m_read.timestamp / 1000.0,
						(double) duration / 1000.0);
				return RD_SUCCESS;
			} else {
				*percent = ((double) rtmp->m_read.timestamp)
						/ (duration * 1000.0) * 100.0;
				*percent = ((double) (int) (*percent * 10.0)) / 10.0;
				RTMP_LogPrintf("%s download at: %.3f kB / %.3f sec (%.1f%%)\n",
						bResume ? "Resuming" : "Starting",
						(double) size / 1024.0,
						(double) rtmp->m_read.timestamp / 1000.0, *percent);
			}
		} else {
			RTMP_LogPrintf("%s download at: %.3f kB\n",
					bResume ? "Resuming" : "Starting", (double) size / 1024.0);
		}
	}

	if (dStopOffset > 0)
		RTMP_LogPrintf("For duration: %.3f sec\n",
				(double) (dStopOffset - dSeek) / 1000.0);

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
	do {
		nRead = RTMP_Read(rtmp, buffer, bufferSize);
		//RTMP_LogPrintf("nRead: %d\n", nRead);
		if (nRead > 0) {
			if (fwrite(buffer, sizeof(unsigned char), nRead, file)
					!= (size_t) nRead) {
				RTMP_Log(RTMP_LOGERROR, "%s: Failed writing, exiting!",
						__FUNCTION__);
				free(buffer);
				return RD_FAILED;
			}
			size += nRead;

			//RTMP_LogPrintf("write %dbytes (%.1f kB)\n", nRead, nRead/1024.0);
			if (duration <= 0) // if duration unknown try to get it from the stream (onMetaData)
				duration = RTMP_GetDuration(rtmp);

			if (duration > 0) {
				// make sure we claim to have enough buffer time!
				if (!bOverrideBufferTime && bufferTime < (duration * 1000.0)) {
					bufferTime = (uint32_t) (duration * 1000.0) + 5000; // extra 5sec to make sure we've got enough

					RTMP_Log(RTMP_LOGDEBUG,
							"Detected that buffer time is less than duration, resetting to: %dms",
							bufferTime);
					RTMP_SetBufferMS(rtmp, bufferTime);
					RTMP_UpdateBufferMS(rtmp);
				}
				*percent = ((double) rtmp->m_read.timestamp)
						/ (duration * 1000.0) * 100.0;
				*percent = ((double) (int) (*percent * 10.0)) / 10.0;
				if (bHashes) {
					if (lastPercent + 1 <= *percent) {
						RTMP_LogStatus("#");
						lastPercent = (unsigned long) *percent;
					}
				} else {
					now = RTMP_GetTime();
					if (abs(now - lastUpdate) > 200) {
						RTMP_LogStatus("\r%.3f kB / %.2f sec (%.1f%%)",
								(double) size / 1024.0,
								(double) (rtmp->m_read.timestamp) / 1000.0,
								*percent);
						lastUpdate = now;
					}
				}
			} else {
				now = RTMP_GetTime();
				if (abs(now - lastUpdate) > 200) {
					if (bHashes)
						RTMP_LogStatus("#");
					else
						RTMP_LogStatus("\r%.3f kB / %.2f sec",
								(double) size / 1024.0,
								(double) (rtmp->m_read.timestamp) / 1000.0);
					lastUpdate = now;
				}
			}
		}
#ifdef _DEBUG
		else
		{
			RTMP_Log(RTMP_LOGDEBUG, "zero read!");
		}
#endif

	} while (!RTMP_ctrlC && nRead > -1 && RTMP_IsConnected(rtmp));
	free(buffer);
	if (nRead < 0)
		nRead = rtmp->m_read.status;

	/* Final status update */
	if (!bHashes) {
		if (duration > 0) {
			*percent = ((double) rtmp->m_read.timestamp) / (duration * 1000.0)
					* 100.0;
			*percent = ((double) (int) (*percent * 10.0)) / 10.0;
			RTMP_LogStatus("\r%.3f kB / %.2f sec (%.1f%%)",
					(double) size / 1024.0,
					(double) (rtmp->m_read.timestamp) / 1000.0, *percent);
		} else {
			RTMP_LogStatus("\r%.3f kB / %.2f sec", (double) size / 1024.0,
					(double) (rtmp->m_read.timestamp) / 1000.0);
		}
	}

	RTMP_Log(RTMP_LOGDEBUG, "RTMP_Read returned: %d", nRead);

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

	return RD_SUCCESS;
}

#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

void usage(char *prog) {
	RTMP_LogPrintf(
			"\n%s: This program dumps the media content streamed over RTMP.\n\n",
			prog);
	RTMP_LogPrintf("--help|-h               Prints this help screen.\n");
	RTMP_LogPrintf(
			"--rtmp|-r url           URL (e.g. rtmp://host[:port]/path)\n");
	RTMP_LogPrintf(
			"--host|-n hostname      Overrides the hostname in the rtmp url\n");
	RTMP_LogPrintf(
			"--port|-c port          Overrides the port in the rtmp url\n");
	RTMP_LogPrintf("--socks|-S host:port    Use the specified SOCKS proxy\n");
	RTMP_LogPrintf(
			"--protocol|-l num       Overrides the protocol in the rtmp url (0 - RTMP, 2 - RTMPE)\n");
	RTMP_LogPrintf(
			"--playpath|-y path      Overrides the playpath parsed from rtmp url\n");
	RTMP_LogPrintf("--playlist|-Y           Set playlist before playing\n");
	RTMP_LogPrintf("--swfUrl|-s url         URL to player swf file\n");
	RTMP_LogPrintf(
			"--tcUrl|-t url          URL to played stream (default: \"rtmp://host[:port]/app\")\n");
	RTMP_LogPrintf("--pageUrl|-p url        Web URL of played programme\n");
	RTMP_LogPrintf("--app|-a app            Name of target app on server\n");
#ifdef CRYPTO
	RTMP_LogPrintf(
			"--swfhash|-w hexstring  SHA256 hash of the decompressed SWF file (32 bytes)\n");
	RTMP_LogPrintf(
			"--swfsize|-x num        Size of the decompressed SWF file, required for SWFVerification\n");
	RTMP_LogPrintf(
			"--swfVfy|-W url         URL to player swf file, compute hash/size automatically\n");
	RTMP_LogPrintf(
			"--swfAge|-X days        Number of days to use cached SWF hash before refreshing\n");
#endif
	RTMP_LogPrintf(
			"--auth|-u string        Authentication string to be appended to the connect string\n");
	RTMP_LogPrintf(
			"--conn|-C type:data     Arbitrary AMF data to be appended to the connect string\n");
	RTMP_LogPrintf(
			"                        B:boolean(0|1), S:string, N:number, O:object-flag(0|1),\n");
	RTMP_LogPrintf(
			"                        Z:(null), NB:name:boolean, NS:name:string, NN:name:number\n");
	RTMP_LogPrintf(
			"--flashVer|-f string    Flash version string (default: \"%s\")\n",
			RTMP_DefaultFlashVer.av_val);
	RTMP_LogPrintf(
			"--live|-v               Save a live stream, no --resume (seeking) of live streams possible\n");
	RTMP_LogPrintf(
			"--subscribe|-d string   Stream name to subscribe to (otherwise defaults to playpath if live is specifed)\n");
	RTMP_LogPrintf(
			"--flv|-o string         FLV output file name, if the file name is - print stream to stdout\n");
	RTMP_LogPrintf("--resume|-e             Resume a partial RTMP download\n");
	RTMP_LogPrintf(
			"--timeout|-m num        Timeout connection num seconds (default: %lu)\n",
			DEF_TIMEOUT);
	RTMP_LogPrintf(
			"--start|-A num          Start at num seconds into stream (not valid when using --live)\n");
	RTMP_LogPrintf("--stop|-B num           Stop at num seconds into stream\n");
	RTMP_LogPrintf("--token|-T key          Key for SecureToken response\n");
	RTMP_LogPrintf(
			"--hashes|-#             Display progress with hashes, not with the byte counter\n");
	RTMP_LogPrintf(
			"--buffer|-b             Buffer time in milliseconds (default: %lu)\n",
			DEF_BUFTIME);
	RTMP_LogPrintf(
			"--skip|-k num           Skip num keyframes when looking for last keyframe to resume from. Useful if resume fails (default: %d)\n\n",
			DEF_SKIPFRM);
	RTMP_LogPrintf("--quiet|-q              Suppresses all command output.\n");
	RTMP_LogPrintf("--verbose|-V            Verbose command output.\n");
	RTMP_LogPrintf("--debug|-z              Debug level command output.\n");
	RTMP_LogPrintf(
			"If you don't pass parameters for swfUrl, pageUrl, or auth these properties will not be included in the connect ");
	RTMP_LogPrintf("packet.\n\n");
}

int main(int argc, char **argv) {
	extern char *optarg;

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
	int initialFrameType = 0; // tye: audio or video

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

	// Check for --quiet option before printing any output
	int index = 0;
	while (index < argc) {
		if (strcmp(argv[index], "--quiet") == 0
				|| strcmp(argv[index], "-q") == 0)
			RTMP_debuglevel = RTMP_LOGCRIT;
		index++;
	}

	RTMP_LogPrintf("RTMPDump %s\n", RTMPDUMP_VERSION);
	RTMP_LogPrintf(
			"(c) 2010 Andrej Stepanchuk, Howard Chu, The Flvstreamer Team; license: GPL\n");

	/* sleep(30); */

	RTMP_Init(&rtmp);

	int opt;
	struct option longopts[] = {
			{ "playpath", 1, NULL, 'y' },
			{ "rtmp", 1, NULL, 'r' },
			{ "pageUrl", 1, NULL, 'p' },
			{ "app", 1, NULL, 'a' },

#ifdef CRYPTO
			{ "swfVfy", 1, NULL, 'W' },
#endif
			 { "flv", 1, NULL, 'o' },
			 { 0, 0, 0, 0 }
			};

	while ((opt = getopt_long(argc, argv,
			"hVveqzr:s:t:p:a:b:f:o:u:C:n:c:l:y:Ym:k:d:A:B:T:w:x:W:X:S:#",
			longopts, NULL)) != -1) {
		switch (opt) {
#ifdef CRYPTO
		case 'W':
			STR2AVAL(swfUrl, optarg);
			swfVfy = 1;
			break;
#endif

		case 'y':
			STR2AVAL(playpath, optarg);
			break;
		case 'r': {
			AVal parsedHost, parsedApp, parsedPlaypath;
			unsigned int parsedPort = 0;
			int parsedProtocol = RTMP_PROTOCOL_UNDEFINED;

			if (!RTMP_ParseURL(optarg, &parsedProtocol, &parsedHost,
					&parsedPort, &parsedPlaypath, &parsedApp)) {
				RTMP_Log(RTMP_LOGWARNING,
						"Couldn't parse the specified url (%s)!", optarg);
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
			break;
		}
		case 'p':
			STR2AVAL(pageUrl, optarg);
			break;
		case 'a':
			STR2AVAL(app, optarg);
			break;
		case 'o':
			flvFile = optarg;
			if (strcmp(flvFile, "-"))
				bStdoutMode = FALSE;

			break;

		default:
			RTMP_LogPrintf("unknown option: %c\n", opt);
			return RD_FAILED;
			break;
		}
	}

	if (!hostname.av_len) {
		RTMP_Log(RTMP_LOGERROR,
				"You must specify a hostname (--host) or url (-r \"rtmp://host[:port]/playpath\") containing a hostname");
		return RD_FAILED;
	}
	if (playpath.av_len == 0) {
		RTMP_Log(RTMP_LOGERROR,
				"You must specify a playpath (--playpath) or url (-r \"rtmp://host[:port]/playpath\") containing a playpath");
		return RD_FAILED;
	}

	if (protocol == RTMP_PROTOCOL_UNDEFINED) {
		RTMP_Log(RTMP_LOGWARNING,
				"You haven't specified a protocol (--protocol) or rtmp url (-r), using default protocol RTMP");
		protocol = RTMP_PROTOCOL_RTMP;
	}
	if (port == -1) {
		RTMP_Log(RTMP_LOGWARNING,
				"You haven't specified a port (--port) or rtmp url (-r), using default port 1935");
		port = 0;
	}
	if (port == 0) {
		if (protocol & RTMP_FEATURE_SSL)
			port = 443;
		else if (protocol & RTMP_FEATURE_HTTP)
			port = 80;
		else
			port = 1935;
	}

	if (flvFile == 0) {
		RTMP_Log(RTMP_LOGWARNING,
				"You haven't specified an output file (-o filename), using stdout");
		bStdoutMode = TRUE;
	}

	if (bStdoutMode && bResume) {
		RTMP_Log(RTMP_LOGWARNING,
				"Can't resume in stdout mode, ignoring --resume option");
		bResume = FALSE;
	}

	if (bLiveStream && bResume) {
		RTMP_Log(RTMP_LOGWARNING,
				"Can't resume live stream, ignoring --resume option");
		bResume = FALSE;
	}

#ifdef CRYPTO
	if (swfVfy) {
		if (RTMP_HashSWF(swfUrl.av_val, &swfSize, hash, swfAge) == 0) {
			swfHash.av_val = (char *) hash;
			swfHash.av_len = RTMP_SWF_HASHLEN;
		}
	}

	if (swfHash.av_len == 0 && swfSize > 0) {
		RTMP_Log(RTMP_LOGWARNING,
				"Ignoring SWF size, supply also the hash with --swfhash");
		swfSize = 0;
	}

	if (swfHash.av_len != 0 && swfSize == 0) {
		RTMP_Log(RTMP_LOGWARNING,
				"Ignoring SWF hash, supply also the swf size  with --swfsize");
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

	// User defined seek offset
	if (dStartOffset > 0) {
		// Live stream
		if (bLiveStream) {
			RTMP_Log(RTMP_LOGWARNING,
					"Can't seek in a live stream, ignoring --start option");
			dStartOffset = 0;
		}
	}

	RTMP_SetupStream(&rtmp, protocol, &hostname, port, &sockshost, &playpath,
			&tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
			&flashVer, &subscribepath, dSeek, dStopOffset, bLiveStream,
			timeout);

	/* Try to keep the stream moving if it pauses on us */
	if (!bLiveStream && !(protocol & RTMP_FEATURE_HTTP))
		rtmp.Link.lFlags |= RTMP_LF_BUFX;

	off_t size = 0;

	if (!file) {
		if (bStdoutMode) {
			file = stdout;
			SET_BINMODE(file);
		} else {
			file = fopen(flvFile, "w+b");
			if (file == 0) {
				RTMP_LogPrintf("Failed to open file! %s\n", flvFile);
				return RD_FAILED;
			}
		}
	}

#ifdef _DEBUG
	netstackdump = fopen("netstackdump", "wb");
	netstackdump_read = fopen("netstackdump_read", "wb");
#endif

	while (!RTMP_ctrlC) {
		RTMP_Log(RTMP_LOGDEBUG, "Setting buffer time to: %dms", bufferTime);
		RTMP_SetBufferMS(&rtmp, bufferTime);

		if (first) {
			first = 0;
			RTMP_LogPrintf("Connecting ...\n");

			if (!RTMP_Connect(&rtmp, NULL)) {
				nStatus = RD_FAILED;
				break;
			}

			RTMP_Log(RTMP_LOGINFO, "Connected...");

			// User defined seek offset
			if (dStartOffset > 0) {
				// Don't need the start offset if resuming an existing file
				if (bResume) {
					RTMP_Log(RTMP_LOGWARNING,
							"Can't seek a resumed stream, ignoring --start option");
					dStartOffset = 0;
				} else {
					dSeek = dStartOffset;
				}
			}

			// Calculate the length of the stream to still play
			if (dStopOffset > 0) {
				// Quit if start seek is past required stop offset
				if (dStopOffset <= dSeek) {
					RTMP_LogPrintf("Already Completed\n");
					nStatus = RD_SUCCESS;
					break;
				}
			}

			if (!RTMP_ConnectStream(&rtmp, dSeek)) {
				nStatus = RD_FAILED;
				break;
			}
		} else {
			nInitialFrameSize = 0;

			if (retries) {
				RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
				if (!RTMP_IsTimedout(&rtmp))
					nStatus = RD_FAILED;
				else
					nStatus = RD_INCOMPLETE;
				break;
			}
			RTMP_Log(RTMP_LOGINFO,
					"Connection timed out, trying to resume.\n\n");
			/* Did we already try pausing, and it still didn't work? */
			if (rtmp.m_pausing == 3) {
				/* Only one try at reconnecting... */
				retries = 1;
				dSeek = rtmp.m_pauseStamp;
				if (dStopOffset > 0) {
					if (dStopOffset <= dSeek) {
						RTMP_LogPrintf("Already Completed\n");
						nStatus = RD_SUCCESS;
						break;
					}
				}
				if (!RTMP_ReconnectStream(&rtmp, dSeek)) {
					RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
					if (!RTMP_IsTimedout(&rtmp))
						nStatus = RD_FAILED;
					else
						nStatus = RD_INCOMPLETE;
					break;
				}
			} else if (!RTMP_ToggleStream(&rtmp)) {
				RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
				if (!RTMP_IsTimedout(&rtmp))
					nStatus = RD_FAILED;
				else
					nStatus = RD_INCOMPLETE;
				break;
			}
			bResume = TRUE;
		}

		nStatus = Download(&rtmp, file, dSeek, dStopOffset, duration, bResume,
				metaHeader, nMetaHeaderSize, initialFrame, initialFrameType,
				nInitialFrameSize, nSkipKeyFrames, bStdoutMode, bLiveStream,
				bHashes, bOverrideBufferTime, bufferTime, &percent);
		free(initialFrame);
		initialFrame = NULL;

		/* If we succeeded, we're done.
		 */
		if (nStatus != RD_INCOMPLETE || !RTMP_IsTimedout(&rtmp) || bLiveStream)
			break;
	}

	if (nStatus == RD_SUCCESS) {
		RTMP_LogPrintf("Download complete\n");
	} else if (nStatus == RD_INCOMPLETE) {
		RTMP_LogPrintf(
				"Download may be incomplete (downloaded about %.2f%%), try resuming\n",
				percent);
	}

	clean: RTMP_Log(RTMP_LOGDEBUG, "Closing connection.\n");
	RTMP_Close(&rtmp);

	if (file != 0)
		fclose(file);

	CleanupSockets();

#ifdef _DEBUG
	if (netstackdump != 0)
	fclose(netstackdump);
	if (netstackdump_read != 0)
	fclose(netstackdump_read);
#endif
	return nStatus;
}
