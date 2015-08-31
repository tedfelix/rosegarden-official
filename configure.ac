
AC_INIT(Rosegarden, 15.08, rosegarden-devel@lists.sourceforge.net)

dnl  Each release will need the proper version  (e.g. 09.10) in the line above
dnl  and the proper codename (e.g. Abraham Darby) in the following line
CODENAME="Nostalgia"
#CODENAME="Oranges and Lemons"
#CODENAME="Peace"
#CODENAME="Querida"
#CODENAME="Rhapsody in Blue"
#CODENAME="Seagull"
#CODENAME="Twice in a Blue Moon"
#CODENAME="Ultimate Pleasure"
#CODENAME="Vesper"
#CODENAME="Worcestershire"
#CODENAME="Xerxes"
#CODENAME="Yesterday"
#CODENAME="Zepherine Drouhin"

AC_CONFIG_SRCDIR(src/base/Composition.h)

# Autoconf will set CXXFLAGS; we don't usually want it to, because we
# either define our own flags (at least if GCC is in use) or else use
# the user's preferences.  We need to ensure CXXFLAGS is only set if
# the user has expressly set it.  So, save the user's (or empty)
# setting now and restore it after Autoconf has done its bit of
# piddling about.
USER_CXXFLAGS="$CXXFLAGS"

AC_LANG_CPLUSPLUS

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P

AC_PATH_X
AC_HEADER_STDC

# These are the flags Autoconf guesses for us; we use them later if
# the user has set none and we are not using GCC (so lack our own
# preferred flags)
AUTOCONF_CXXFLAGS="$CXXFLAGS"

PKG_PROG_PKG_CONFIG

RG_CHECK_QT

AC_CHECK_LIB([X11],[XSetErrorHandler],[LIBS="$LIBS -lX11"],[AC_MSG_ERROR(Failed to find required X11 library)])
AC_CHECK_LIB([z],[gzopen],[LIBS="$LIBS -lz"],[AC_MSG_ERROR(Failed to find required libz library)])
AC_CHECK_LIB([dl],[dlopen],[LIBS="$LIBS -ldl"],[AC_MSG_ERROR(Failed to find required dl library)])

JACK_FOUND="0"
AC_CHECK_LIB(
    [jack],[jack_client_open],
    [LIBS="$LIBS -ljack" HAVES="$HAVES -DHAVE_LIBJACK" JACK_FOUND="1"],
    [AC_MSG_WARN(No JACK library found: audio will be unavailable!)])

AC_CHECK_HEADER([ladspa.h],[],[AC_MSG_ERROR(Failed to find required LADSPA header ladspa.h)])

# This little bit of juggling is to enable DSSI to be included even on
# systems without ALSA (dssi.h includes an ALSA header, but we can
# fill in the gap if ALSA itself is not present)
DSSI_LOCALFLAGS=""
PKG_CHECK_MODULES([ALSA],[alsa >= 0.9],[HAVES="$HAVES -DHAVE_ALSA"],[
AC_MSG_WARN(No ALSA library found: MIDI will be unavailable!)
AC_MSG_NOTICE(Using compatibility code for DSSI event types)
DSSI_LOCALFLAGS="-Isrc/sound/dssi-compatibility"
CXXFLAGS="$CXXFLAGS $DSSI_LOCALFLAGS"
])
AC_CHECK_HEADER([dssi.h],[],[AC_MSG_ERROR(Failed to find required DSSI header dssi.h)])

AC_CHECK_HEADER([X11/SM/SMlib.h],[],[AC_MSG_ERROR(Failed to find required X11 header X11/SM/SMlib.h)])

AC_C_BIGENDIAN([AC_SUBST(ENDIANNESS,"-DBIG_ENDIAN=1")],[AC_SUBST(ENDIANNESS,"-DLITTLE_ENDIAN=1")],[AC_MSG_ERROR(Could not find system endianess)])

PKG_CHECK_MODULES([liblo],[liblo >= 0.7],[],[])
PKG_CHECK_MODULES([lrdf],[lrdf >= 0.2],[],[])
PKG_CHECK_MODULES([fftw3f],[fftw3f >= 3.0.0],[],[])
PKG_CHECK_MODULES([sndfile],[sndfile >= 1.0.16],[HAVES="$HAVES -DHAVE_LIBSNDFILE"],[])
PKG_CHECK_MODULES([samplerate],[samplerate >= 0.1.2],[],[])

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
       	AC_MSG_ERROR([Failed to find required makedepend program.  (It is sometimes provided by the "imake" package.)])
fi

AC_PATH_PROG([SHA1SUM],[sha1sum])
AC_PATH_PROG([SHA1SUM],[sha1 -q])
if test x$SHA1SUM = x ; then
       	AC_MSG_ERROR([Failed to find required sha1sum or sha1 program.])
fi

AC_PATH_PROG([CUT],[cut])
if test x$CUT = x ; then
       	AC_MSG_ERROR([Failed to find required cut program.])
fi

# Do this last, so that the warning is visible (we will repeat any
# really vital warnings, i.e. those for ALSA and JACK, afterward)
AC_CHECK_HEADER([lirc/lirc_client.h],[HAVES="$HAVES -DHAVE_LIRC"],[AC_MSG_WARN(Failed to find LIRC header lirc/lirc_client.h)])
AC_CHECK_LIB([lirc_client],[lirc_init],[LIBS="$LIBS -llirc_client"],[AC_MSG_WARN(Failed to find LIRC library lirc_client)])

RG_DEFINES_DEBUG="-DDEBUG -DBUILD_DEBUG -DWANT_TIMING"
RG_DEFINES_RELEASE="-DNDEBUG -DQT_NO_DEBUG -DBUILD_RELEASE -DNO_TIMING"
RG_DEFINES_MINIMAL="$RG_DEFINES_RELEASE"

# Now we have: USER_CXXFLAGS contains any flags the user set
# explicitly; AUTOCONF_CXXFLAGS contains flags that Autoconf thought
# we should use.  If we have GCC, we override the latter but then
# allow ourselves to be overridden (later) by the former

CXXFLAGS_DEBUG="$AUTOCONF_CXXFLAGS"
CXXFLAGS_RELEASE="$AUTOCONF_CXXFLAGS"
CXXFLAGS_MINIMAL="$AUTOCONF_CXXFLAGS"

if test "x$GCC" = "xyes"; then
        CXXFLAGS_DEBUG="-Wall -Woverloaded-virtual -Wextra -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -g -pipe"
   	CXXFLAGS_RELEASE="-g0 -O2 -Wall -pipe"
   	CXXFLAGS_MINIMAL="-g0 -O0"
fi

CXXFLAGS_BUILD="$CXXFLAGS_RELEASE"
RG_DEFINES_BUILD="$RG_DEFINES_RELEASE"

AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug support [default=no]])],[AC_MSG_NOTICE([enabling debug build])
CXXFLAGS_BUILD="$CXXFLAGS_DEBUG"
RG_DEFINES_BUILD="$RG_DEFINES_DEBUG"])

if test x"$USER_CXXFLAGS" != x; then
   	AC_MSG_NOTICE([The CXXFLAGS environment variable is set to "$USER_CXXFLAGS".])
	AC_MSG_NOTICE(Overriding default compiler flags with the above user setting.)
	CXXFLAGS_BUILD="$USER_CXXFLAGS"
	CXXFLAGS_MINIMAL="$USER_CXXFLAGS"
fi

CXXFLAGS="$CXXFLAGS_BUILD $RG_DEFINES_BUILD"

if test "$JACK_FOUND" = "0" ; then
   AC_MSG_WARN([Required JACK library not found, building with no audio!])
fi
if test -z "$ALSA_CFLAGS" ; then
   AC_MSG_WARN([Required ALSA library not found, building with no MIDI!])
fi

AC_SUBST(PERL)
AC_SUBST(XARGS)
AC_SUBST(MAKEDEPEND)
AC_SUBST(SHA1SUM)
AC_SUBST(CUT)

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(CXXFLAGS_MINIMAL)
AC_SUBST(HAVES)
AC_SUBST(DSSI_LOCALFLAGS)

AC_SUBST(CC)
AC_SUBST(CXX)

AC_SUBST(CODENAME)

AC_OUTPUT([Makefile])

