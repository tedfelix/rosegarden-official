
AC_INIT(Rosegarden, 09.06, rosegarden-devel@lists.sourceforge.net)

AC_CONFIG_SRCDIR(src/base/Composition.h)

AC_LANG_CPLUSPLUS

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
AC_CHECK_HEADER([lirc/lirc_client.h],[HAVES="$HAVES -DHAVE_LIRC"],[AC_MSG_ERROR(Failed to find required LIRC header lirc/lirc_client.h)])
AC_CHECK_LIB([lirc_client],[lirc_init],[LIBS="$LIBS -llirc_client"],[AC_MSG_ERROR(Failed to find required LIRC library lirc_client)])

# hm, we'll probably want a newer version for jack_client_open
PKG_CHECK_MODULES([JACK],[jack >= 0.77],[HAVES="$HAVES -DHAVE_LIBJACK"],[])
PKG_CHECK_MODULES([ALSA],[alsa >= 0.9],[HAVES="$HAVES -DHAVE_ALSA"],[])
PKG_CHECK_MODULES([DSSI],[dssi >= 0.9],[],[])
PKG_CHECK_MODULES([liblo],[liblo >= 0.7],[],[])
PKG_CHECK_MODULES([lrdf],[lrdf >= 0.2],[],[])
PKG_CHECK_MODULES([Xft],[xft >= 2.0],[HAVES="$HAVES -DHAVE_XFT"],[])
PKG_CHECK_MODULES([fftw3f],[fftw3f >= 3.0.0],[],[])

AC_PATH_PROG([PERL],[perl])
if test x$PERL = x ; then
       	AC_MSG_ERROR([
Failed to find required perl program.
])
fi

AC_PATH_PROG([XARGS],[xargs])
if test x$XARGS = x ; then
       	AC_MSG_ERROR([
Failed to find required xargs program.
])
fi

AC_PATH_PROG([MAKEDEPEND],[makedepend])
if test x$MAKEDEPEND = x ; then
       	AC_MSG_ERROR([
Failed to find required makedepend program.
])
fi

BUILDFLAGS="-DNDEBUG -DNO_TIMING -DBUILD_RELEASE"
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug support [default=no]])],[AC_MSG_NOTICE([enabling debug build])
BUILDFLAGS="-DDEBUG -O0 -DBUILD_DEBUG -DWANT_TIMING"])
CXXFLAGS="$CXXFLAGS $BUILDFLAGS"

AC_SUBST(PERL)
AC_SUBST(XARGS)
AC_SUBST(MAKEDEPEND)

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(HAVES)

AC_SUBST(CC)
AC_SUBST(CXX)

AC_OUTPUT([Makefile])

