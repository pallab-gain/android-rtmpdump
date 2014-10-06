VERSION=v2.3

prefix=/usr/local

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld

SYS=posix
#SYS=mingw

CRYPTO=OPENSSL
#CRYPTO=POLARSSL
#CRYPTO=GNUTLS
LIB_GNUTLS=-lgnutls -lgcrypt
LIB_OPENSSL=-lssl -lcrypto
LIB_POLARSSL=-lpolarssl
CRYPTO_LIB=$(LIB_$(CRYPTO))
DEF_=-DNO_CRYPTO
CRYPTO_DEF=$(DEF_$(CRYPTO))

DEF=-DRTMPDUMP_VERSION=\"$(VERSION)\" $(CRYPTO_DEF) $(XDEF)
OPT=-O2
CFLAGS=-Wall $(XCFLAGS) $(INC) $(DEF) $(OPT)
LDFLAGS=-Wall $(XLDFLAGS)

bindir=$(prefix)/bin
sbindir=$(prefix)/sbin
mandir=$(prefix)/man

BINDIR=$(DESTDIR)$(bindir)
SBINDIR=$(DESTDIR)$(sbindir)
MANDIR=$(DESTDIR)$(mandir)

LIBS_posix=
LIBS_mingw=-lws2_32 -lwinmm -lgdi32
LIBS=$(CRYPTO_LIB) -lz $(LIBS_$(SYS)) $(XLIBS)

THREADLIB_posix=-lpthread
THREADLIB_mingw=
THREADLIB=$(THREADLIB_$(SYS))
SLIBS=$(THREADLIB) $(LIBS)

LIBRTMP=librtmp/librtmp.a
INCRTMP=librtmp/rtmp_sys.h librtmp/rtmp.h librtmp/log.h librtmp/amf.h

EXT_posix=
EXT_mingw=.exe
EXT=$(EXT_$(SYS))

all:	$(LIBRTMP) progs

progs:	rtmpdump

install:	progs
	-mkdir -p $(BINDIR) $(SBINDIR) $(MANDIR)/man1 $(MANDIR)/man8
	cp rtmpdump$(EXT) $(BINDIR)
	@cd librtmp; $(MAKE) install

clean:
	rm -f *.o rtmpdump$(EXT)
	@cd librtmp; $(MAKE) clean

FORCE:

$(LIBRTMP): FORCE
	@cd librtmp; $(MAKE) all

# note: $^ is GNU Make's equivalent to BSD $>
# we use both since either make will ignore the one it doesn't recognize

rtmpdump: rtmpdump.o $(LIBRTMP)
	$(CC) $(LDFLAGS) $^ $> -o $@$(EXT) $(LIBS)
rtmpdump.o: rtmpdump.c $(INCRTMP) Makefile
