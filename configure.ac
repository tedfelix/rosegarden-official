
AC_INIT(Rosegarden, 10.02, rosegarden-devel@lists.sourceforge.net)

dnl  Each release will need the proper version  (e.g. 09.10) in the line above
dnl  and the proper codename (e.g. Abraham Darby) in the following line
CODENAME="Thorn"

AC_CONFIG_SRCDIR(src/base/Composition.h)

AC_LANG_CPLUSPLUS

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P

AC_PATH_X
AC_HEADER_STDC

PKG_PROG_PKG_CONFIG

RG_CHECK_QT

# the following just test for things, they don't yet set the proper
# flags or bail out

AC_CHECK_HEADER([ladspa.h],[],[AC_MSG_ERROR(Failed to find required LADSPA header ladspa.h)])
AC_CHECK_HEADER([X11/SM/SMlib.h],[],[AC_MSG_ERROR(Failed to find required X11 header X11/SM/SMlib.h)])

AC_CHECK_HEADER([lirc/lirc_client.h],[HAVES="$HAVES -DHAVE_LIRC"],[AC_MSG_ERROR(Failed to find required LIRC header lirc/lirc_client.h)])
AC_CHECK_LIB([lirc_client],[lirc_init],[LIBS="$LIBS -llirc_client"],[AC_MSG_ERROR(Failed to find required LIRC library lirc_client)])

AC_C_BIGENDIAN([AC_SUBST(ENDIANNESS,"-DBIG_ENDIAN=1")],[AC_SUBST(ENDIANNESS,"-DLITTLE_ENDIAN=1")],[AC_MSG_ERROR(Could not find system endianess)])
# hm, we'll probably want a newer version for jack_client_open
PKG_CHECK_MODULES([JACK],[jack >= 0.77],[HAVES="$HAVES -DHAVE_LIBJACK"],[])
PKG_CHECK_MODULES([ALSA],[alsa >= 0.9],[HAVES="$HAVES -DHAVE_ALSA"],[])
PKG_CHECK_MODULES([DSSI],[dssi >= 0.9],[],[])
PKG_CHECK_MODULES([liblo],[liblo >= 0.7],[],[])
PKG_CHECK_MODULES([lrdf],[lrdf >= 0.2],[],[])
PKG_CHECK_MODULES([fftw3f],[fftw3f >= 3.0.0],[],[])
PKG_CHECK_MODULES([sndfile],[sndfile >= 1.0.16],[HAVES="$HAVES -DHAVE_LIBSNDFILE"],[])
PKG_CHECK_MODULES([samplerate],[samplerate >= 0.1.4],[],[])

AC_PATH_PROG([PERL],[perl])
if test x$PERL = x ; then
       	AC_MSG_ERROR([Failed to find required perl program.])
fi

AC_PATH_PROG([XARGS],[xargs])
if test x$XARGS = x ; then
       	AC_MSG_ERROR([Failed to find required xargs program.])
fi

AC_PATH_PROG([MAKEDEPEND],[makedepend])
if test x$MAKEDEPEND = x ; then
       	AC_MSG_ERROR([Failed to find required makedepend program.])
fi

AC_PATH_PROG([SHA1SUM],[sha1sum])
if test x$SHA1SUM = x ; then
       	AC_MSG_ERROR([Failed to find required sha1sum program.])
fi

AC_PATH_PROG([CUT],[cut])
if test x$CUT = x ; then
       	AC_MSG_ERROR([Failed to find required cut program.])
fi

DEBUGFLAGS="-DDEBUG -DBUILD_DEBUG -DWANT_TIMING"
RELEASEFLAGS="-DNDEBUG -DBUILD_RELEASE -DNO_TIMING"
MINIMAL_CFLAGS="$RELEASEFLAGS"

if test "x$GCC" = "xyes"; then
#	DEBUGFLAGS="$DEBUGFLAGS -Wall -Woverloaded-virtual -Wextra -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -O1 -g -pipe"
	DEBUGFLAGS="$DEBUGFLAGS -Wall -Woverloaded-virtual -Wextra -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -O0 -g -pipe"
	RELEASEFLAGS="$RELEASEFLAGS -g0 -O2 -Wall -pipe"
	MINIMAL_CFLAGS="$MINIMAL_CFLAGS -g0 -O0"
fi

BUILDFLAGS="$RELEASEFLAGS"
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug support [default=no]])],[AC_MSG_NOTICE([enabling debug build])
BUILDFLAGS="$DEBUGFLAGS"])
#CXXFLAGS="$CXXFLAGS $BUILDFLAGS"
CXXFLAGS="$BUILDFLAGS"

AC_SUBST(PERL)
AC_SUBST(XARGS)
AC_SUBST(MAKEDEPEND)
AC_SUBST(SHA1SUM)
AC_SUBST(CUT)

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(HAVES)
AC_SUBST(MINIMAL_CFLAGS)

AC_SUBST(CC)
AC_SUBST(CXX)

AC_SUBST(CODENAME)

AC_OUTPUT([Makefile])

