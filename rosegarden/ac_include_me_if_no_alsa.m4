# If you're building from CVS and have the following error :

# aclocal: configure.in: 86: macro `AM_PATH_ALSA' not found in library

# and then configure crashes, call

# aclocal -I .
# 
# and restart make -f Makefile.cvs ; configure [ your options ]
#

AC_DEFUN(AM_PATH_ALSA, [

with_alsa="no"
ALSA_CFLAGS=""
ALSA_LIBS=""

AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)

])
